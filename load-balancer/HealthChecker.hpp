#pragma once

#include "ServerDetails.hpp"
#include "LbClientManager.hpp"
#include "ThreadPool.hpp"
#include <asio/steady_timer.hpp>
#include <vector>
#include <optional>
#include <mutex>
#include <spdlog/spdlog.h>
#include <memory>

namespace kload_balancer
{
class HealthChecker
{
    std::shared_ptr<ThreadPool> m_thread_pool;
    asio::steady_timer m_timer;

    std::int_least64_t m_period_in_seconds;
    std::shared_ptr<LbClientManager> m_lb_client_manager;

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
    HealthChecker(std::shared_ptr<ThreadPool> thread_pool, std::shared_ptr<LbClientManager> lb_client_manager, std::int_least64_t period_in_seconds);
    ~HealthChecker();
    void start();
    std::optional<ServerDetails> get_next_available_server();
    void add_server(std::string_view ip_address, std::string_view port);

};
}   // namespace kload_balancer