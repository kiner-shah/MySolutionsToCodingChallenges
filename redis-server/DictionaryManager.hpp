#pragma once

#include <unordered_map>
#include <mutex>
#include <array>
#include <vector>
#include <string>
#include <memory>
#include <thread>
#include <random>
#include <optional>
#include <asio/io_context.hpp>
#include <asio/steady_timer.hpp>
#include "DictionaryValue.hpp"
#include "RdbManager.hpp"

namespace kredis
{
class DictionaryManager : public std::enable_shared_from_this<DictionaryManager>
{
    using DictionaryType = std::unordered_map<std::string, DictionaryValue>;

    asio::io_context m_io_context;
    asio::executor_work_guard<asio::io_context::executor_type> m_work_guard;
    std::thread m_io_context_thread;
    asio::steady_timer m_timer;
    std::uint64_t m_expiry_checking_period_seconds;
    std::random_device m_random_device;
    std::mt19937 m_rng_engine;

    struct DictionaryShard
    {
        DictionaryType m_dictionary;
        mutable std::mutex m_mutex;
    };
    std::array<DictionaryShard, 8> m_dictionary_shards;
    mutable std::mutex m_save_mutex;

    RdbManager m_rdb_manager;

    std::size_t get_shard_index(const std::string& key) const;
    void set_shard_entry(const std::string& key, DictionaryValue value);
    std::size_t remove_shard_entry(const std::string& key);
    void try_remove_expired_keys();
    void handle_timer_complete(const asio::error_code& error);
    void stop();
    bool has_expired(const DictionaryValue& value) const;
public:
    DictionaryManager(std::uint64_t expiry_checking_period_seconds);
    ~DictionaryManager();
    void start();
    void set(const std::string& key, RespTypePtr value, std::optional<std::uint64_t> timestamp = std::nullopt);
    RespTypePtr get(const std::string& key);
    RespTypePtr increment(const std::string& key);
    RespTypePtr decrement(const std::string& key);
    bool exists(const std::string& key);
    std::size_t remove(const std::string& key);
    RespTypePtr push_front(const std::string& key, const std::vector<RespString>& values);
    RespTypePtr push_back(const std::string& key, const std::vector<RespString>& values);
    RespTypePtr get_list(const std::string& key, int start_offset, int end_offset);
    bool save();
    void load();
};
}   // namespace kredis