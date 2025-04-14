#include "LoadBalancer.hpp"
#include <asio/placeholders.hpp>
#include <asio/connect.hpp>
#include <spdlog/sinks/stdout_color_sinks.h>
#include "utils.hpp"

namespace kload_balancer
{
LoadBalancer::LoadBalancer(std::string_view ip_address, std::string_view port, std::int_least64_t health_check_period_in_seconds)
    : m_signals{m_io_context}, m_acceptor{m_io_context}, m_ip_address{std::move(ip_address)},
    m_port{std::move(port)}, m_user_client_count{0}, m_logger{spdlog::stdout_color_mt("LoadBalancer")},
    m_health_checker{std::make_unique<HealthChecker>(health_check_period_in_seconds)}
{
    m_logger->set_level(spdlog::level::debug);
    m_logger->set_pattern("%Y-%m-%d %H:%M:%S | %n | %-8l | %v");

    m_signals.add(SIGINT);
    m_signals.add(SIGTERM);

    m_signals.async_wait([this](std::error_code error, int signal_no)
    {
        stop();
    });

    asio::ip::tcp::resolver endpoint_resolver{m_acceptor.get_executor()};
    asio::ip::tcp::endpoint endpoint = *endpoint_resolver.resolve(ip_address, port).begin();
    m_acceptor.open(endpoint.protocol());
    m_acceptor.set_option(asio::ip::tcp::acceptor::reuse_address{true});
    m_acceptor.bind(endpoint);
    m_acceptor.listen();

    m_unused_clients_deleter = std::thread([this]() {
        while (!m_is_stopped)
        {
            std::unique_lock<std::mutex> lock_outer{m_to_delete_clients_mutex};
            m_to_delete_clients_cv.wait(lock_outer, [this]() { return !m_to_delete_clients.empty(); });

            auto id = m_to_delete_clients.front();
            m_to_delete_clients.pop();
            lock_outer.unlock();

            {
                std::scoped_lock<std::mutex> lock_inner{m_user_clients_mutex};
                auto it = std::find_if(m_user_clients.begin(), m_user_clients.end(),
                    [&id](const UserClientPtr& p)
                    {
                        return p->get_id() == id;
                    });
                if (it != m_user_clients.end())
                {
                    std::string id = (*it)->get_id();
                    m_user_clients.erase(it);
                    m_logger->debug("Deleted [UserClientId {}]", id);
                }
            }
        }
    });

    m_unused_servers_deleter = std::thread([this]() {
        while (!m_is_stopped)
        {
            std::unique_lock<std::mutex> lock_outer{m_to_delete_servers_mutex};
            m_to_delete_servers_cv.wait(lock_outer, [this]() { return !m_to_delete_servers.empty(); });

            auto ip_port = m_to_delete_servers.front();
            m_to_delete_servers.pop();
            lock_outer.unlock();

            {
                std::scoped_lock<std::mutex> lock_inner{m_clients_mutex};
                auto it = std::find_if(m_clients.begin(), m_clients.end(),
                    [&ip_port](const ClientPtr& p)
                    {
                        return p->get_ip_port() == ip_port;
                    });
                if (it != m_clients.end())
                {
                    std::string ip_port = (*it)->get_ip_port();
                    m_clients.erase(it);
                    m_logger->debug("Deleted [ClientIpPort {}]", ip_port);
                }
            }
        }
    });

    m_health_checker->start();

    accept();
}

LoadBalancer::~LoadBalancer()
{
    if (!m_is_stopped)
    {
        stop();
    }
    m_unused_clients_deleter.join();
    m_unused_servers_deleter.join();
}

void LoadBalancer::start()
{
    m_io_context.run();
}

void LoadBalancer::stop()
{
    m_is_stopped = true;
    m_io_context.stop();
    {
        std::unique_lock<std::mutex> lock_user_clients{m_user_clients_mutex};
        bool is_empty = m_user_clients.empty();
        for (const auto& ptr : m_user_clients)
        {
            std::scoped_lock<std::mutex> lock{m_to_delete_clients_mutex};
            m_to_delete_clients.push(ptr->get_id());
            m_to_delete_clients_cv.notify_one();
        }
        lock_user_clients.unlock();
        if (is_empty)
        {
            std::scoped_lock<std::mutex> lock{m_to_delete_clients_mutex};
            m_to_delete_clients.push("DummyUserClient");
            m_to_delete_clients_cv.notify_one();
        }
    }
    {
        std::unique_lock<std::mutex> lock_clients{m_clients_mutex};
        bool is_empty = m_clients.empty();
        for (const auto& ptr : m_clients)
        {
            std::scoped_lock<std::mutex> lock{m_to_delete_servers_mutex};
            m_to_delete_servers.push(ptr->get_ip_port());
            m_to_delete_servers_cv.notify_one();
        }
        lock_clients.unlock();
        if (is_empty)
        {
            std::scoped_lock<std::mutex> lock{m_to_delete_servers_mutex};
            m_to_delete_servers.push("DummyIpPort");
            m_to_delete_servers_cv.notify_one();
        }
    }
}

void LoadBalancer::add_server(std::string_view ip_address, std::string_view port)
{
    m_health_checker->add_server(std::move(ip_address), std::move(port));
}

void LoadBalancer::accept()
{
    std::scoped_lock<std::mutex> user_client_lock(m_user_clients_mutex);
    std::string new_user_client_id = "Client_" + std::to_string(m_user_client_count);
    m_user_client_count++;
    m_user_clients.emplace_back(std::make_unique<UserClient>(new_user_client_id,
        [this](std::array<unsigned char, 2048> read_data, asio::error_code error, std::size_t bytes_transferred, std::string user_client_id)
        {
            on_client_read_done(read_data, error, bytes_transferred, user_client_id);
        },
        [this](std::array<unsigned char, 2048> write_data, asio::error_code error, std::size_t bytes_transferred, std::string user_client_id)
        {
            on_client_write_done(write_data, error, bytes_transferred, user_client_id);
        })
    );
    m_acceptor.async_accept(m_user_clients.back()->get_socket(),
        [this, new_user_client_id](const asio::error_code &error)
        {
            handle_accept(error, new_user_client_id);
            accept();
        }
    );
}

void LoadBalancer::handle_accept(const asio::error_code &error, std::string user_client_id)
{
    // Check whether the server was stopped by a signal before this
    // completion handler had a chance to run.
    if (!m_acceptor.is_open())
    {
        return;
    }

    m_logger->debug("New connection request [UserClientId {}]", user_client_id);
    {
        std::unique_lock<std::mutex> user_client_lock(m_user_clients_mutex);
        auto it = std::find_if(m_user_clients.begin(), m_user_clients.end(),
            [&user_client_id](const UserClientPtr& user_client_ptr)
            {
                return user_client_ptr->get_id() == user_client_id;
            }
        );
        if (it == m_user_clients.end())
        {
            return;
        }
        if (!error)
        {
            m_logger->debug("Waiting for read to complete for from [UserClientId {}]", user_client_id);
            (*it)->read();
        }
        else
        {
            user_client_lock.unlock();
            std::scoped_lock<std::mutex> lock{m_to_delete_clients_mutex};
            m_to_delete_clients.push(user_client_id);
            m_to_delete_clients_cv.notify_one();
        }
    }
}

void LoadBalancer::on_client_read_done(std::array<unsigned char, 2048> read_data, asio::error_code error, std::size_t bytes_transferred, std::string user_client_id)
{
    if (error)
    {
        return;
    }
    m_logger->debug("Received message from [UserClientId {}]:\n{}", user_client_id, std::string{read_data.begin(), read_data.end()});
    // Get next available server
    auto next_available_server = m_health_checker->get_next_available_server();
    if (!next_available_server)
    {
        std::scoped_lock<std::mutex> user_client_lock(m_user_clients_mutex);

        // No server available, write error message to the client
        std::array<unsigned char, 2048> buffer = get_service_unavailable_message();
        auto it = std::find_if(m_user_clients.begin(), m_user_clients.end(),
            [&user_client_id](const UserClientPtr& user_client_ptr)
            {
                return user_client_ptr->get_id() == user_client_id;
            }
        );
        if (it == m_user_clients.end())
        {
            m_logger->warn("Client not found in list {}", user_client_id);
            return;
        }
        m_logger->debug("No servers available [UserClientId {}]", user_client_id);
        (*it)->write(buffer);
        return;
    }
    m_logger->debug("Connecting to server {}:{} for [UserClientId {}]", next_available_server.value().get_ip(), next_available_server.value().get_port(), user_client_id);
    {
        // establish connection to server
        std::scoped_lock<std::mutex> client_lock(m_clients_mutex);
        // TODO: if connect() fails it will throw an std::system_error which is not caught here.
        //       Not sure what needs to be done when that happens.
        m_clients.emplace_back(std::make_unique<Client>(next_available_server.value().get_ip(), next_available_server.value().get_port(),
            [this](std::array<unsigned char, 2048> data, asio::error_code error, std::size_t bytes_transferred, std::string user_client_id, std::string server_endpoint)
            {
                on_server_read_done(data, error, bytes_transferred, user_client_id, server_endpoint);
            },
            [this](std::array<unsigned char, 2048> data, asio::error_code error, std::size_t bytes_transferred, std::string user_client_id, std::string server_endpoint)
            {
                on_server_write_done(data, error, bytes_transferred, user_client_id, server_endpoint);
            }));
        // write to the server
        m_logger->debug("Waiting for write to complete for [ClientIpPort {}]", m_clients.back()->get_ip_port());
        m_clients.back()->write(read_data, std::move(user_client_id));
    }
}

void LoadBalancer::on_client_write_done(std::array<unsigned char, 2048> write_data, asio::error_code, std::size_t, std::string user_client_id)
{
    m_logger->debug("Sent message to [UserClientId {}]:\n{}", user_client_id, std::string{write_data.begin(), write_data.end()});
    // close connection to client, remove client
    m_logger->debug("Deleting client [UserClientId {}]", user_client_id);
    {
        std::scoped_lock<std::mutex> lock{m_to_delete_clients_mutex};
        m_to_delete_clients.push(user_client_id);
        m_to_delete_clients_cv.notify_one();
    }
}

void LoadBalancer::on_server_read_done(std::array<unsigned char, 2048> read_data, asio::error_code error, std::size_t bytes_transferred, std::string user_client_id, std::string server_endpoint)
{
    if (error)
    {
        // Something went wrong while getting response from server.
        // This is an internal server error, write the appropriate message.
        m_logger->error("Internal server error: {} {} {} {} {}", error.message(), bytes_transferred, std::string{read_data.begin(), read_data.end()}, user_client_id, server_endpoint);
        read_data = get_internal_server_error_message();
    }
    {
        std::scoped_lock<std::mutex> user_client_lock(m_user_clients_mutex);
        // write to the client
        auto cit = std::find_if(m_user_clients.begin(), m_user_clients.end(),
            [&user_client_id](const UserClientPtr& user_client_ptr)
            {
                return user_client_ptr->get_id() == user_client_id;
            }
        );
        if (cit == m_user_clients.end())
        {
            m_logger->error("Client not found in list {}", user_client_id);
            return;
        }
        m_logger->debug("Waiting for write to complete for [UserClientId {}]", user_client_id);
        (*cit)->write(read_data);
    }
    {
        // close connection to server
        m_logger->debug("Deleting server [ClientIpPort {}] for [UserClientId {}]", server_endpoint, user_client_id);
        {
            std::scoped_lock<std::mutex> lock{m_to_delete_servers_mutex};
            m_to_delete_servers.push(server_endpoint);
            m_to_delete_servers_cv.notify_one();
        }
    }
}

void LoadBalancer::on_server_write_done(std::array<unsigned char, 2048>, asio::error_code error, std::size_t, std::string user_client_id, std::string server_endpoint)
{
    if (error)
    {
        std::scoped_lock<std::mutex> user_client_lock(m_user_clients_mutex);

        // TODO: is this an internal server error?
        std::array<unsigned char, 2048> buffer;
        for (std::size_t i = 0; i < error.message().size(); i++)
        {
            buffer[i] = error.message().at(i);
        }
        auto it = std::find_if(m_user_clients.begin(), m_user_clients.end(),
            [&user_client_id](const UserClientPtr& user_client_ptr)
            {
                return user_client_ptr->get_id() == user_client_id;
            }
        );
        if (it == m_user_clients.end())
        {
            m_logger->warn("Client not found in list {}", user_client_id);
            return;
        }
        m_logger->debug("Waiting for write to complete for [UserClientId {}]", user_client_id);
        (*it)->write(buffer);
    }

    {
        std::scoped_lock<std::mutex> client_lock(m_clients_mutex);
        // read from server
        auto sit = std::find_if(m_clients.begin(), m_clients.end(),
            [&server_endpoint](const ClientPtr& client_ptr)
            {
                return client_ptr->get_ip_port() == server_endpoint;
            }
        );
        if (sit == m_clients.end())
        {
            m_logger->warn("Server not found in list {}", server_endpoint);
            return;
        }
        m_logger->debug("Waiting for read to complete for [ClientIpPort {}]", server_endpoint);
        (*sit)->read(std::move(user_client_id));
    }
}
} // namespace kload_balancer