#include "LoadBalancer.hpp"
#include <asio/placeholders.hpp>
#include <asio/connect.hpp>
#include "utils.hpp"
#include <iostream>

namespace kload_balancer
{
LoadBalancer::ServerDetails::ServerDetails(std::string_view ip, std::string_view port)
    : m_ip{std::move(ip)}, m_port{std::move(port)}, m_is_available{false}
{
}

bool LoadBalancer::ServerDetails::is_available() const
{
    return m_is_available;
}

std::string_view LoadBalancer::ServerDetails::get_ip() const
{
    return m_ip;
}

std::string_view LoadBalancer::ServerDetails::get_port() const
{
    return m_port;
}

void LoadBalancer::ServerDetails::set_available(bool status)
{
    m_is_available = status;
}

LoadBalancer::LoadBalancer(unsigned short port)
    : m_signals{m_io_context}, m_acceptor{m_io_context},
    m_last_server_index{0}, m_port{port}, m_user_client_count{0}
{
    m_signals.add(SIGINT);
    m_signals.add(SIGTERM);

    m_signals.async_wait([this](std::error_code error, int signal_no)
    {
        stop();
    });

    // TODO: bind the acceptor and listen

    accept();
}

LoadBalancer::~LoadBalancer()
{
    if (!m_is_stopped)
    {
        stop();
    }
}

void LoadBalancer::start()
{
    m_io_context.run();
}

void LoadBalancer::stop()
{
    m_io_context.stop();
    m_io_context.reset();
    m_user_clients.erase(m_user_clients.begin(), m_user_clients.end());
    m_clients.erase(m_clients.begin(), m_clients.end());
    m_is_stopped = true;
}

void LoadBalancer::add_server(std::string_view ip_address, std::string_view port)
{
    m_servers.emplace_back(std::move(ip_address), std::move(port));
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
        [this](std::array<unsigned char, 2048> read_data, asio::error_code error, std::size_t bytes_transferred, std::string user_client_id)
        {
            on_client_write_done(read_data, error, bytes_transferred, user_client_id);
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

    {
        std::scoped_lock<std::mutex> user_client_lock(m_user_clients_mutex);
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
            (*it)->read();
        }
        else
        {
            m_user_clients.erase(it);
        }
    }
}

bool LoadBalancer::get_next_available_server(std::size_t &index)
{
    index = m_last_server_index;
    std::size_t unavailable_count = 0;
    // TODO: implement health check
    while (!m_servers[index].is_available())
    {
        index = (index + 1) % m_servers.size();
        unavailable_count++;
        if (unavailable_count == m_servers.size())
        {
            return false;
        }
    }
    return true;
}

void LoadBalancer::on_client_read_done(std::array<unsigned char, 2048> read_data, asio::error_code error, std::size_t bytes_transferred, std::string user_client_id)
{
    if (error)
    {
        return;
    }
    // get_next_available_server
    std::size_t server_index{0};
    if (!get_next_available_server(server_index))
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
            return;
        }
        (*it)->write(buffer);
        return;
    }
    {
        std::scoped_lock<std::mutex> client_lock(m_clients_mutex);
        // establish connection to server
        m_clients.emplace_back(std::make_unique<Client>(m_servers[server_index].get_ip(), m_servers[server_index].get_port(),
            [this](std::array<unsigned char, 2048> data, asio::error_code error, std::size_t bytes_transferred, std::string user_client_id, std::string server_endpoint)
            {
                on_server_read_done(data, error, bytes_transferred, user_client_id, server_endpoint);
            },
            [this](std::array<unsigned char, 2048> data, asio::error_code error, std::size_t bytes_transferred, std::string user_client_id, std::string server_endpoint)
            {
                on_server_write_done(data, error, bytes_transferred, user_client_id, server_endpoint);
            }));
        // write to the server
        m_clients.back()->write(read_data, std::move(user_client_id));
    }
}

void LoadBalancer::on_client_write_done(std::array<unsigned char, 2048>, asio::error_code, std::size_t, std::string user_client_id)
{
    std::scoped_lock<std::mutex> user_client_id_lock(m_user_clients_mutex);
    // close connection to client, remove client
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
    m_user_clients.erase(it);
}

void LoadBalancer::on_server_read_done(std::array<unsigned char, 2048> read_data, asio::error_code error, std::size_t, std::string user_client_id, std::string server_endpoint)
{
    if (error)
    {
        // Something went wrong while getting response from server.
        // This is an internal server error, write the appropriate message.
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
            return;
        }
        (*cit)->write(read_data);
    }
    {
        std::scoped_lock<std::mutex> client_lock(m_clients_mutex);
        // close connection to server
        auto sit = std::find_if(m_clients.begin(), m_clients.end(),
            [&server_endpoint](const ClientPtr& client_ptr)
            {
                return client_ptr->get_ip_port() == server_endpoint;
            }
        );
        if (sit == m_clients.end())
        {
            return;
        }
        m_clients.erase(sit);
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
            return;
        }
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
            return;
        }
        (*sit)->read(std::move(user_client_id));
    }
}
} // namespace kload_balancer