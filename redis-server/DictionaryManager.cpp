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
void DictionaryManager::try_remove_expired_keys()
{
    std::shared_lock<std::shared_mutex> lock{m_shared_mutex};

    using ReferenceToEntry = std::reference_wrapper<const std::pair<const std::string, DictionaryValue>>;

    std::vector<ReferenceToEntry> entries_with_expiry;
    for (const auto& entry : m_dictionary)
    {
        if (entry.second.m_expiry_timestamp.has_value())
        {
            entries_with_expiry.push_back(std::cref(entry));
        }
    }

    std::vector<ReferenceToEntry> random_sample;
    std::sample(entries_with_expiry.begin(),
        entries_with_expiry.end(),
        std::back_inserter(random_sample),
        5,  // sample size is 5
        std::mt19937{std::random_device{}()});
    
    std::size_t expired_count = 0;
    std::vector<std::string> keys_to_remove;
    for (const auto& entry : random_sample)
    {
        if (has_expired(entry.get().second))
        {
            keys_to_remove.push_back(entry.get().first);
            expired_count++;
        }
    }
    lock.unlock();
    for (const auto& key : keys_to_remove)
    {
        remove(key);
    }
    if (expired_count >= 2)
    {
        try_remove_expired_keys();
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
    auto current_timestamp = get_current_time<std::chrono::milliseconds>();
    return current_timestamp > value.m_expiry_timestamp;
}

DictionaryManager::DictionaryManager(std::uint64_t expiry_checking_period_seconds)
    : m_work_guard{asio::make_work_guard(m_io_context)},
    m_timer{m_io_context, std::chrono::seconds(expiry_checking_period_seconds)},
    m_expiry_checking_period_seconds{expiry_checking_period_seconds}
{
    m_io_context_thread = std::thread([this]() { m_io_context.run(); });
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
    std::unique_lock<std::shared_mutex> lock{m_shared_mutex};
    m_dictionary[key] = DictionaryValue {std::move(value), timestamp};
}

RespTypePtr DictionaryManager::get(const std::string &key)
{
    std::shared_lock<std::shared_mutex> read_lock{m_shared_mutex};
    auto it = m_dictionary.find(key);
    if (it == m_dictionary.end())
    {
        return std::make_shared<RespType>(RespNull{});
    }
    m_timer.cancel();
    if (has_expired(it->second))
    {
        read_lock.unlock();
        remove(key);
        m_timer.async_wait([self = shared_from_this()](const asio::error_code& error)
        {
            self->handle_timer_complete(error);
        });
        return std::make_shared<RespType>(RespNull{});
    }
    return it->second.m_value;
}

void DictionaryManager::remove(const std::string &key)
{
    std::unique_lock<std::shared_mutex> lock{m_shared_mutex};
    m_dictionary.erase(key);
}
} // namespace kredis