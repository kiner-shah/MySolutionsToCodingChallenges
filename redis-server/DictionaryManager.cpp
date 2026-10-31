#include "DictionaryManager.hpp"
#include "RespErrors.hpp"
#include "utils.hpp"
#include <asio/post.hpp>
#include <mutex>
#include <random>
#include <limits>
#include <iostream>

namespace kredis
{
std::size_t DictionaryManager::get_shard_index(const std::string &key) const
{
    return std::hash<std::string>{}(key) % m_dictionary_shards.size();
}

void DictionaryManager::set_shard_entry(const std::string &key, DictionaryValue value)
{
    auto& shard = m_dictionary_shards[get_shard_index(key)];
    {
        std::scoped_lock<std::mutex> lock{shard.m_mutex};
        auto& entry = shard.m_dictionary[key];
        std::swap(entry, value);
    }
}

std::size_t DictionaryManager::remove_shard_entry(const std::string &key)
{
    auto& shard = m_dictionary_shards[get_shard_index(key)];
    {
        std::scoped_lock<std::mutex> lock{shard.m_mutex};
        return shard.m_dictionary.erase(key);
    }
}

std::optional<std::reference_wrapper<const DictionaryValue>> DictionaryManager::get_shard_entry(const std::string &key)
{
    const auto& shard = m_dictionary_shards[get_shard_index(key)];
    {
        std::scoped_lock<std::mutex> lock{shard.m_mutex};
        auto it = shard.m_dictionary.find(key);
        if (it == shard.m_dictionary.end())
        {
            return std::nullopt;
        }
        return std::cref(it->second);
    }
}

void DictionaryManager::try_remove_expired_keys()
{
    std::uniform_int_distribution<std::size_t> dist{0, m_dictionary_shards.size() - 1};
    std::size_t shard_index_to_inspect = dist(m_rng_engine);
    std::size_t loop_count = 0;
    const std::size_t loop_max = 10;
    while (loop_count < loop_max)
    {
        auto& shard = m_dictionary_shards[shard_index_to_inspect];
        bool continue_same_shard = true;
        while (continue_same_shard)
        {
            std::unique_lock<std::mutex> lock{shard.m_mutex};
            std::size_t inspect_count = 0;
            const std::size_t max_inspect_count = 15;
            std::vector<std::string> keys_to_remove{};

            std::uniform_int_distribution<std::size_t> bdist{0, shard.m_dictionary.bucket_count()};
            std::size_t random_bucket_index = bdist(m_rng_engine);

            for (std::size_t bucket_index = 0; bucket_index < shard.m_dictionary.bucket_count(); bucket_index++)
            {
                if (inspect_count >= max_inspect_count)
                {
                    break;
                }
                std::size_t current_bucket_index = (bucket_index + random_bucket_index) % shard.m_dictionary.bucket_count();
                for (auto it = shard.m_dictionary.begin(current_bucket_index); it != shard.m_dictionary.end(current_bucket_index); it++)
                {
                    if (inspect_count >= max_inspect_count)
                    {
                        break;
                    }
                    ++inspect_count;
                    if (has_expired(it->second))
                    {
                        keys_to_remove.push_back(it->first);
                    }
                }
            }
            if (inspect_count < 5)
            {
                continue_same_shard = false;
                break;
            }

            for (const auto& key : keys_to_remove)
            {
                shard.m_dictionary.erase(key);
            }
            bool threshold_exceeded = keys_to_remove.size() * 100 > inspect_count * 20;
            if (!threshold_exceeded)
            {
                continue_same_shard = false;
            }
        }
        shard_index_to_inspect = dist(m_rng_engine);
        ++loop_count;
    }
}

void DictionaryManager::handle_timer_complete(const asio::error_code &error)
{
    if (!error)
    {
        try_remove_expired_keys();

        m_timer.expires_at(m_timer.expiry() + std::chrono::seconds(m_expiry_checking_period_seconds));
        m_timer.async_wait([self = shared_from_this()](const asio::error_code& error)
        {
            self->handle_timer_complete(error);
        });
    }
}

void DictionaryManager::stop()
{
    m_work_guard.reset();
    while (!m_io_context.stopped())
    {
        m_io_context.stop();
    }
    m_io_context_thread.join();
}

bool DictionaryManager::has_expired(const DictionaryValue& value) const
{
    if (!value.m_expiry_timestamp.has_value())
    {
        return false;
    }
    auto current_timestamp = get_current_time<std::chrono::milliseconds>();
    return current_timestamp > value.m_expiry_timestamp;
}

DictionaryManager::DictionaryManager(std::uint64_t expiry_checking_period_seconds)
    : m_work_guard{asio::make_work_guard(m_io_context)},
    m_timer{m_io_context, std::chrono::seconds(expiry_checking_period_seconds)},
    m_expiry_checking_period_seconds{expiry_checking_period_seconds},
    m_rng_engine{m_random_device()}
{
    m_io_context_thread = std::thread([this]() { m_io_context.run(); });
    load();
}

DictionaryManager::~DictionaryManager()
{
    stop();
}

void DictionaryManager::start()
{
    m_timer.async_wait([self = shared_from_this()](const asio::error_code& error)
    {
        self->handle_timer_complete(error);
    });
}

void DictionaryManager::set(const std::string &key,
    RespTypePtr value,
    std::optional<std::uint64_t> timestamp)
{
    set_shard_entry(key, DictionaryValue{std::move(value), timestamp});
}

RespTypePtr DictionaryManager::get(const std::string &key)
{
    auto optional_entry = get_shard_entry(key);
    if (!optional_entry.has_value())
    {
        return std::make_shared<RespType>(RespNull{});
    }
    auto& entry = optional_entry->get();

    // TODO: is it needed to cancel here? Can the timer be kept running?
    // m_timer.cancel();
    if (has_expired(entry))
    {
        remove(key);
        return std::make_shared<RespType>(RespNull{});
    }
    if (!std::holds_alternative<RespString>(*entry.m_value))
    {
        return std::make_shared<RespType>(RespError{invalid_operation});
    }
    return entry.m_value;
}

RespTypePtr DictionaryManager::increment(const std::string &key)
{
    // If exists and is a number, then increment
    // If not exists or expired, then set to 1
    auto optional_entry = get_shard_entry(key);
    if (!optional_entry.has_value())
    {
        return std::make_shared<RespType>(RespNull{});
    }
    auto& entry = optional_entry->get();

    // TODO: is it needed to cancel here? Can the timer be kept running?
    // m_timer.cancel();
    if (has_expired(entry))
    {
        auto value_ptr = std::make_shared<RespType>(RespInt{1});
        set(key, value_ptr);
        return value_ptr;
    }
    if (!std::holds_alternative<RespString>(*entry.m_value))
    {
        return std::make_shared<RespType>(RespError{invalid_operation});
    }
    RespString value = std::get<RespString>(*entry.m_value);
    auto int_value_opt = is_valid_signed_64_bit_int(value.m_str);
    if (!int_value_opt.has_value())
    {
        return std::make_shared<RespType>(RespError{invalid_operation});
    }
    // TODO: check for overflow
    auto new_value_ptr = std::make_shared<RespType>(RespString{std::to_string(int_value_opt.value() + 1), false});
    set(key, new_value_ptr, entry.m_expiry_timestamp);
    return new_value_ptr;
}

RespTypePtr DictionaryManager::decrement(const std::string &key)
{
    // If exists and is a number, then decrement
    // If not exists or expired, then set to 1
    auto optional_entry = get_shard_entry(key);
    if (!optional_entry.has_value())
    {
        return std::make_shared<RespType>(RespNull{});
    }
    auto& entry = optional_entry->get();

    // TODO: is it needed to cancel here? Can the timer be kept running?
    // m_timer.cancel();
    if (has_expired(entry))
    {
        auto value_ptr = std::make_shared<RespType>(RespInt{1});
        set(key, value_ptr);
        return value_ptr;
    }
    if (!std::holds_alternative<RespString>(*entry.m_value))
    {
        return std::make_shared<RespType>(RespError{invalid_operation});
    }
    RespString value = std::get<RespString>(*entry.m_value);
    auto int_value_opt = is_valid_signed_64_bit_int(value.m_str);
    if (!int_value_opt.has_value())
    {
        return std::make_shared<RespType>(RespError{invalid_operation});
    }
    // TODO: check for underflow
    auto new_value_ptr = std::make_shared<RespType>(RespString{std::to_string(int_value_opt.value() - 1), false});
    set(key, new_value_ptr, entry.m_expiry_timestamp);
    return new_value_ptr;
}

bool DictionaryManager::exists(const std::string &key)
{
    return get_shard_entry(key).has_value();
}

std::size_t DictionaryManager::remove(const std::string &key)
{
    return remove_shard_entry(key);
}

RespTypePtr DictionaryManager::push_front(const std::string &key, const std::vector<RespString> &values)
{
    auto optional_entry = get_shard_entry(key);
    if (!optional_entry.has_value())
    {
        RespArray new_list;
        for (const auto& val : values)
        {
            new_list.push_front(val);
        }
        set_shard_entry(key, DictionaryValue{ std::make_shared<RespType>(new_list) });
        return std::make_shared<RespType>(RespInt{ static_cast<RespInt>(values.size()) });
    }
    else
    {
        auto& entry = optional_entry->get();
        if (!std::holds_alternative<RespArray>(*entry.m_value))
        {
            return std::make_shared<RespType>(RespError{invalid_operation});
        }
        auto& existing_list = std::get<RespArray>(*entry.m_value);
        for (const auto& val : values)
        {
            existing_list.push_front(val);
        }
        return std::make_shared<RespType>(RespInt{ static_cast<RespInt>(existing_list.size()) });
    }
}

RespTypePtr DictionaryManager::push_back(const std::string &key, const std::vector<RespString> &values)
{
    auto optional_entry = get_shard_entry(key);
    if (!optional_entry.has_value())
    {
        RespArray new_list;
        for (const auto& val : values)
        {
            new_list.push_back(val);
        }
        set_shard_entry(key, DictionaryValue{ std::make_shared<RespType>(new_list) });
        return std::make_shared<RespType>(RespInt{ static_cast<RespInt>(values.size()) });
    }
    else
    {
        auto& entry = optional_entry->get();
        if (!std::holds_alternative<RespArray>(*entry.m_value))
        {
            return std::make_shared<RespType>(RespError{invalid_operation});
        }
        auto& existing_list = std::get<RespArray>(*entry.m_value);
        for (const auto& val : values)
        {
            existing_list.push_back(val);
        }
        return std::make_shared<RespType>(RespInt{ static_cast<RespInt>(existing_list.size()) });
    }
}

RespTypePtr DictionaryManager::get_list(const std::string &key, int start_offset, int end_offset)
{
    auto optional_entry = get_shard_entry(key);
    if (!optional_entry.has_value())
    {
        return std::make_shared<RespType>(RespArray{});
    }
    auto& entry = optional_entry->get();


    if (!std::holds_alternative<RespArray>(*entry.m_value))
    {
        return std::make_shared<RespType>(RespError{invalid_operation});
    }
    const auto& existing_list = std::get<RespArray>(*entry.m_value);
    if (start_offset < 0)
    {
        start_offset = existing_list.size() + start_offset;
    }
    if (end_offset < 0)
    {
        end_offset = existing_list.size() + end_offset;
    }

    RespArray result_list{};
    if (start_offset > end_offset || start_offset >= static_cast<int>(existing_list.size()))
    {
        return std::make_shared<RespType>(result_list);
    }
    start_offset = std::max(0, start_offset);
    end_offset = std::min(end_offset, static_cast<int>(existing_list.size() - 1));
    for (int index = start_offset; index <= end_offset; index++)
    {
        result_list.push_back(existing_list[index]);
    }
    return std::make_shared<RespType>(result_list);
}

bool DictionaryManager::save()
{
    std::vector<std::pair<std::string, DictionaryValue>> dictionary_snapshot;
    for (const auto& shard : m_dictionary_shards)
    {
        std::scoped_lock<std::mutex> lock{shard.m_mutex};
        for (const auto& entry : shard.m_dictionary)
        {
            dictionary_snapshot.push_back(entry);
        }
    }
    return m_rdb_manager.save(dictionary_snapshot);
}

void DictionaryManager::load()
{
    auto snapshot = m_rdb_manager.load();
    if (snapshot.has_value())
    {
        for (const auto& entry : *snapshot)
        {
            set_shard_entry(entry.first, entry.second);
        }
    }
}
} // namespace kredis