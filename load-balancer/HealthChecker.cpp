#include "HealthChecker.hpp"
#include "utils.hpp"
#include <spdlog/sinks/stdout_color_sinks.h>

namespace kload_balancer
{
HealthChecker::HealthChecker(std::int_least64_t period_in_seconds)
    : m_work_guard{asio::make_work_guard(m_io_context)}, m_timer{m_io_context, std::chrono::seconds(period_in_seconds)},
    m_period_in_seconds{period_in_seconds}, m_last_server_index{0}, m_logger{spdlog::stdout_color_mt("HealthChecker")}
{
    m_logger->set_level(spdlog::level::debug);
    m_logger->set_pattern("%Y-%m-%d %H:%M:%S | %n | %-8l | %v");

    m_unused_clients_deleter = std::thread([this]()
        {
            while (!m_is_stopped)
            {    
                std::unique_lock<std::mutex> lock_outer{m_to_delete_clients_mutex};
                m_to_delete_clients_cv.wait(lock_outer, [this]() { return !m_to_delete_clients.empty(); });

                auto ip_port = m_to_delete_clients.front();
                m_to_delete_clients.pop();
                lock_outer.unlock();

                {
                    std::scoped_lock<std::mutex> lock_inner{m_clients_mutex};
                    auto it = std::find_if(m_clients.begin(), m_clients.end(),
                        [&ip_port](const ClientPtr& p)
                        {
                            return p->get_ip_port() == ip_port;
                        }
                    );
                    if (it != m_clients.end())
                    {
                        m_clients.erase(it);
                    }
                }
            }
        }
    );

    m_io_context_thread = std::thread([this]() { m_io_context.run(); });
}

HealthChecker::~HealthChecker()
{
    if (!m_is_stopped)
    {
        stop();
    }
    m_unused_clients_deleter.join();
}

void HealthChecker::start()
{
    m_timer.async_wait([this](const asio::error_code& error) { handle_timer_complete(error); });
}

void HealthChecker::stop()
{
    m_is_stopped = true;
    m_work_guard.reset();
    while (!m_io_context.stopped())
    {
        m_io_context.stop();
    }

    m_io_context_thread.join();

    std::unique_lock<std::mutex> lock_clients{m_clients_mutex};
    bool is_empty = m_clients.empty();
    for (const auto& ptr : m_clients)
    {
        std::scoped_lock<std::mutex> lock{m_to_delete_clients_mutex};
        m_to_delete_clients.push(ptr->get_ip_port());
        m_to_delete_clients_cv.notify_one();
    }
    lock_clients.unlock();
    if (is_empty)
    {
        std::scoped_lock<std::mutex> lock{m_to_delete_clients_mutex};
        m_to_delete_clients.push("DummyIpPort");
        m_to_delete_clients_cv.notify_one();
    }
}

std::optional<ServerDetails> HealthChecker::get_next_available_server()
{
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
            std::scoped_lock<std::mutex> lock{m_clients_mutex};
            m_clients.emplace_back(std::make_unique<Client>(server.get_ip(), server.get_port(),
                [this](std::array<unsigned char, 2048> data, asio::error_code error, std::size_t bytes_transferred, std::string user_client_id, std::string server_endpoint)
                {
                    on_server_read_done(data, error, bytes_transferred, user_client_id, server_endpoint);
                },
                [this](std::array<unsigned char, 2048> data, asio::error_code error, std::size_t bytes_transferred, std::string user_client_id, std::string server_endpoint)
                {
                    on_server_write_done(data, error, bytes_transferred, user_client_id, server_endpoint);
                })
            );
            std::array<unsigned char, 2048> buffer = construct_health_check_message(server);
            m_clients.back()->write(buffer, "HealthChecker");
        }
        catch (const std::system_error& e)
        {
            m_logger->debug("Error during health check: {}", e.what());
            server.set_available(false);
        }
    }
}

void HealthChecker::on_server_write_done(std::array<unsigned char, 2048> buffer, asio::error_code error, std::size_t, std::string client_id, std::string server_endpoint)
{
    m_logger->debug("Sent health check message to server {}:\n{}", server_endpoint, std::string{buffer.begin(), buffer.end()});
    std::unique_lock<std::mutex> client_lock{m_clients_mutex};
    auto it = std::find_if(m_clients.begin(), m_clients.end(),
        [&server_endpoint](const ClientPtr& server)
        {
            return server->get_ip_port() == server_endpoint;
        }
    );
    if (it != m_clients.end())
    {
        (*it)->read(client_id);
        client_lock.unlock();
        return;
    }
    {
        std::scoped_lock<std::mutex> server_lock{m_servers_mutex};
        auto sit = std::find_if(m_servers.begin(), m_servers.end(),
            [&server_endpoint](const ServerDetails& server)
            {
                return server.get_endpoint() == server_endpoint;
            }
        );
        if (sit != m_servers.end())
        {
            (*sit).set_available(false);
        }
    }
}

void HealthChecker::on_server_read_done(std::array<unsigned char, 2048> buffer, asio::error_code error, std::size_t, std::string, std::string server_endpoint)
{
    m_logger->debug("Received response from server {}:\n{}", server_endpoint, std::string{buffer.begin(), buffer.end()});
    std::unique_lock<std::mutex> server_lock{m_servers_mutex};
    auto sit = std::find_if(m_servers.begin(), m_servers.end(),
        [&server_endpoint](const ServerDetails& server)
        {
            return server.get_endpoint() == server_endpoint;
        }
    );
    if (sit != m_servers.end())
    {
        (*sit).set_available(error ? false : true);
        m_logger->debug("Server health {} {}", (*sit).get_endpoint(), (*sit).is_available() ? "available" : "not available");
    }
    server_lock.unlock();
    {
        std::scoped_lock<std::mutex> lock{m_to_delete_clients_mutex};
        m_to_delete_clients.push(server_endpoint);
        m_to_delete_clients_cv.notify_one();
    }
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