#pragma once

#include <unordered_map>
#include <shared_mutex>
#include <string>
#include <memory>
#include <optional>
#include <thread>
#include <asio/io_context.hpp>
#include <asio/steady_timer.hpp>
#include "RespTypes.hpp"

namespace kredis
{
struct DictionaryValue
{
    RespTypePtr m_value;
    std::optional<std::uint64_t> m_expiry_timestamp;
};

class DictionaryManager : public std::enable_shared_from_this<DictionaryManager>
{
    using DictionaryType = std::unordered_map<std::string, DictionaryValue>;

    asio::io_context m_io_context;
    asio::executor_work_guard<asio::io_context::executor_type> m_work_guard;
    std::thread m_io_context_thread;
    asio::steady_timer m_timer;
    std::uint64_t m_expiry_checking_period_seconds;

    DictionaryType m_dictionary;
    mutable std::shared_mutex m_shared_mutex;

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
    void remove(const std::string& key);
};
}   // namespace kredis