#pragma once

#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/signal_set.hpp>
#include <memory>
#include <string_view>
#include <vector>
// #include <queue>
// #include <mutex>
// #include <condition_variable>
#include <atomic>
#include <spdlog/spdlog.h>
#include "UserClient.hpp"
#include "LbClient.hpp"
#include "HealthChecker.hpp"
#include "UserClientManager.hpp"
#include "LbClientManager.hpp"
#include "ThreadPool.hpp"

namespace kload_balancer
{
class LoadBalancer
{
    std::shared_ptr<ThreadPool> m_thread_pool;
    asio::signal_set m_signals;
    asio::ip::tcp::acceptor m_acceptor;
    std::atomic_bool m_is_stopped = false;
    std::string_view m_ip_address;
    std::string_view m_port;

    std::shared_ptr<spdlog::logger> m_logger;

    std::unique_ptr<UserClientManager> m_user_client_manager;
    std::shared_ptr<LbClientManager> m_lb_client_manager;
    std::unique_ptr<HealthChecker> m_health_checker;

    void handle_accept(const asio::error_code& error, std::string user_client_id);
    void on_client_read_done(std::array<unsigned char, 2048>, asio::error_code, std::size_t, std::string);
    void on_client_write_done(std::array<unsigned char, 2048>, asio::error_code, std::size_t, std::string);
    void on_server_read_done(std::array<unsigned char, 2048>, asio::error_code, std::size_t, std::string, std::string);
    void on_server_write_done(std::array<unsigned char, 2048>, asio::error_code, std::size_t, std::string, std::string);
public:
    LoadBalancer(std::string_view ip_address, std::string_view port, std::int_least64_t health_check_period_in_seconds);
    ~LoadBalancer();
    void start();
    void stop();
    void add_server(std::string_view ip_address, std::string_view port);
    void accept();
};
}   // namespace kload_balancer