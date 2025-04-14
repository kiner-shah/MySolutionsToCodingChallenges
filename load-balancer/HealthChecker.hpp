#pragma once

#include "ServerDetails.hpp"
#include "Client.hpp"
#include <asio/io_context.hpp>
#include <asio/steady_timer.hpp>
#include <optional>
#include <queue>
#include <vector>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <spdlog/spdlog.h>

namespace kload_balancer
{
class HealthChecker
{
    using ClientPtr = std::unique_ptr<Client>;

    asio::io_context m_io_context;
    asio::executor_work_guard<asio::io_context::executor_type> m_work_guard;
    std::thread m_io_context_thread;
    asio::steady_timer m_timer;

    std::queue<std::string> m_to_delete_clients;
    std::mutex m_to_delete_clients_mutex;
    std::condition_variable m_to_delete_clients_cv;
    std::thread m_unused_clients_deleter;
    std::int_least64_t m_period_in_seconds;

    std::vector<ClientPtr> m_clients;
    std::mutex m_clients_mutex;

    std::atomic_bool m_is_stopped = false;
    std::vector<ServerDetails> m_servers;
    std::mutex m_servers_mutex;

    std::size_t m_last_server_index;

    std::shared_ptr<spdlog::logger> m_logger;

    void perform_health_check();
    void on_server_write_done(std::array<unsigned char, 2048>, asio::error_code, std::size_t, std::string, std::string);
    void on_server_read_done(std::array<unsigned char, 2048>, asio::error_code, std::size_t, std::string, std::string);
    void handle_timer_complete(const asio::error_code& error);
    void stop();
public:
    HealthChecker(std::int_least64_t period_in_seconds);
    ~HealthChecker();
    void start();
    std::optional<ServerDetails> get_next_available_server();
    void add_server(std::string_view ip_address, std::string_view port);

};
}   // namespace kload_balancer