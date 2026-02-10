#include "HealthChecker.hpp"
#include "utils.hpp"
#include <spdlog/sinks/stdout_color_sinks.h>

namespace kload_balancer
{
HealthChecker::HealthChecker(
    std::shared_ptr<ThreadPool> thread_pool,
    std::shared_ptr<LbClientManager> lb_client_manager,
    std::int_least64_t period_in_seconds)
    : m_thread_pool{std::move(thread_pool)},
    m_timer{m_thread_pool->get_io_context(), std::chrono::seconds(period_in_seconds)},
    m_period_in_seconds{period_in_seconds},
    m_lb_client_manager{std::move(lb_client_manager)},
    m_last_server_index{0},
    m_logger{spdlog::stdout_color_mt("HealthChecker")}
{
    m_logger->set_level(spdlog::level::debug);
    m_logger->set_pattern("%Y-%m-%d %H:%M:%S | %n | %-8l | %v");
}

HealthChecker::~HealthChecker()
{
    if (!m_is_stopped)
    {
        stop();
    }
}

void HealthChecker::start()
{
    m_timer.async_wait([this](const asio::error_code& error) { handle_timer_complete(error); });
}

void HealthChecker::stop()
{
    m_timer.cancel();
    m_is_stopped = true;
}

std::optional<ServerDetails> HealthChecker::get_next_available_server()
{
    std::scoped_lock<std::mutex> server_lock{m_servers_mutex};
    std::size_t index = (m_last_server_index + 1) % m_servers.size();
    std::size_t unavailable_count = 0;
    while (!m_servers[index].is_available())
    {
        index = (index + 1) % m_servers.size();
        unavailable_count++;
        if (unavailable_count == m_servers.size())
        {
            return std::nullopt;
        }
    }
    m_last_server_index = index;
    return m_servers[m_last_server_index];
}

void HealthChecker::add_server(std::string_view ip_address, std::string_view port)
{
    std::scoped_lock<std::mutex> server_lock{m_servers_mutex};
    m_servers.emplace_back(ip_address, port);
}

void HealthChecker::perform_health_check()
{
    m_logger->info("Running health check");
    std::scoped_lock<std::mutex> server_lock{m_servers_mutex};
    for (auto& server : m_servers)
    {
        try
        {
            auto lb_client_ptr = m_lb_client_manager->create_new_lb_client(
                m_thread_pool->get_io_context(),
                server.get_ip(),
                server.get_port(),
                std::bind(&HealthChecker::on_server_read_done, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5),
                std::bind(&HealthChecker::on_server_write_done, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5),
                true
            );
            std::array<unsigned char, 2048> buffer = construct_health_check_message(server);
            lb_client_ptr->write(buffer, "HealthChecker");
        }
        catch (const std::system_error& e)
        {
            m_logger->debug("Error during health check: {}", e.what());
            server.set_available(false);
        }
    }
}

void HealthChecker::on_server_write_done(std::array<unsigned char, 2048> buffer, asio::error_code error, std::size_t, std::string client_id, std::string server_id)
{
    m_logger->debug("Sent health check message to server {}:\n{}", server_id, std::string{buffer.begin(), buffer.end()});
    auto lb_client_ptr = m_lb_client_manager->get_lb_client(server_id);
    if (lb_client_ptr)
    {
        lb_client_ptr->read(client_id);
        return;
    }
    {
        std::scoped_lock<std::mutex> server_lock{m_servers_mutex};
        auto sit = std::find_if(m_servers.begin(), m_servers.end(),
            [&lb_client_ptr](const ServerDetails& server)
            {
                return server.get_endpoint() == lb_client_ptr->get_ip_port();
            }
        );
        if (sit != m_servers.end())
        {
            (*sit).set_available(false);
        }
    }
}

void HealthChecker::on_server_read_done(std::array<unsigned char, 2048> buffer, asio::error_code error, std::size_t, std::string, std::string server_id)
{
    m_logger->debug("Received response from server {}:\n{}", server_id, std::string{buffer.begin(), buffer.end()});
    auto lb_client_ptr = m_lb_client_manager->get_lb_client(server_id);
    if (!lb_client_ptr)
    {
        m_logger->warn("Server not found in list {}", server_id);
        return;
    }
    {
        std::scoped_lock<std::mutex> server_lock{m_servers_mutex};
        auto sit = std::find_if(m_servers.begin(), m_servers.end(),
            [&lb_client_ptr](const ServerDetails& server)
            {
                return server.get_endpoint() == lb_client_ptr->get_ip_port();
            }
        );
        if (sit != m_servers.end())
        {
            (*sit).set_available(error ? false : true);
            m_logger->debug("Server health {} {}", (*sit).get_endpoint(), (*sit).is_available() ? "available" : "not available");
        }
    }
    m_lb_client_manager->remove_lb_client(server_id);
}

void HealthChecker::handle_timer_complete(const asio::error_code &error)
{
    if (!error)
    {
        perform_health_check();
        m_timer.expires_at(m_timer.expiry() + std::chrono::seconds(m_period_in_seconds));
        m_timer.async_wait([this](const asio::error_code& error) { handle_timer_complete(error); });
    }
}
} // namespace kload_balancer