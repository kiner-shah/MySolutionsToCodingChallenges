#include "Server.hpp"
#include <spdlog/sinks/stdout_color_sinks.h>
#include "RespGenerator.hpp"

namespace kredis
{
void Server::handle_accept(const asio::error_code &error, std::string client_id)
{
    // Check whether the server was stopped by a signal before this
    // completion handler had a chance to run.
    if (!m_acceptor.is_open())
    {
        return;
    }

    m_logger->debug("New connection request [ClientId {}]", client_id);
    {
        std::unique_lock<std::mutex> client_lock{m_clients_mutex};
        auto it = std::find_if(m_clients.begin(), m_clients.end(),
            [&client_id](const ClientPtr& client)
            {
                return client->get_id() == client_id;
            }
        );
        if (it == m_clients.end())
        {
            return;
        }
        if (!error)
        {
            m_logger->debug("Waiting for read to complete for from [ClientId {}, Local endpoint address {}]",
                client_id,
                (*it)->get_socket().local_endpoint().address().to_string());
            (*it)->read();
        }
        else
        {
            client_lock.unlock();
            std::scoped_lock<std::mutex> lock{m_to_delete_clients_mutex};
            m_to_delete_clients.push(client_id);
            m_to_delete_clients_cv.notify_one();
        }
    }
}

void Server::on_read_done(std::array<unsigned char, 2048> read_data, asio::error_code error, std::size_t bytes_transferred, std::string client_id)
{
    if (error)
    {
        if (error == asio::error::connection_reset || error == asio::error::eof)
        {
            // Client is no longer connected, remove from clients list
            std::scoped_lock<std::mutex> lock{m_to_delete_clients_mutex};
            m_to_delete_clients.push(client_id);
            m_to_delete_clients_cv.notify_one();
        }
        else
        {
            m_logger->warn("Error occured during read: {}", error.message());
        }
        return;
    }
    m_logger->debug("Received message from [ClientId {}]:\n{}", client_id, std::string{read_data.begin(), read_data.end()});
    // TODO:
    // Do some processing

    // Write back to client
    // TODO: below is dummy code, remove later
    RespGenerator generator{m_logger};
    std::string response = generator.generate(RespString{"PONG", false});
    {
        std::scoped_lock<std::mutex> lock{m_clients_mutex};
        auto it = std::find_if(m_clients.begin(), m_clients.end(),
            [&client_id](const ClientPtr& client)
            {
                return client->get_id() == client_id;
            });
        if (it != m_clients.end())
        {
            std::array<unsigned char, 2048> buffer;
            std::copy(response.begin(), response.end(), buffer.begin());
            (*it)->write(buffer);
        }
    }
}

void Server::on_write_done(std::array<unsigned char, 2048> write_data, asio::error_code error, std::size_t bytes_transferred, std::string client_id)
{
    if (error)
    {
        if (error == asio::error::connection_reset || error == asio::error::eof)
        {
            // Client is no longer connected, remove from clients list
            std::scoped_lock<std::mutex> lock{m_to_delete_clients_mutex};
            m_to_delete_clients.push(client_id);
            m_to_delete_clients_cv.notify_one();
        }
        else
        {
            m_logger->warn("Error occured during write: {}", error.message());
        }
        return;
    }
    m_logger->debug("Sent message to [ClientId {}]:\n{}", client_id, std::string{write_data.begin(), write_data.end()});
}

Server::Server()
    : m_signals{m_io_context}, m_acceptor{m_io_context}, m_logger{spdlog::stdout_color_mt("RedisServer")}, m_client_count{0}
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
    asio::ip::tcp::endpoint endpoint = *endpoint_resolver.resolve("0.0.0.0", "6379").begin();
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
                std::scoped_lock<std::mutex> lock_inner{m_clients_mutex};
                auto it = std::find_if(m_clients.begin(), m_clients.end(),
                    [&id](const ClientPtr& client)
                    {
                        return client->get_id() == id;
                    });
                if (it != m_clients.end())
                {
                    std::string id = (*it)->get_id();
                    m_clients.erase(it);
                    m_logger->debug("Deleted [ClientId {}]", id);
                }
            }
        }
    });

    accept();
}

Server::~Server()
{
    if (!m_is_stopped)
    {
        stop();
    }
    m_unused_clients_deleter.join();
}

void Server::start()
{
    m_io_context.run();
}

void Server::stop()
{
    m_is_stopped = true;
    m_io_context.stop();
    {
        std::unique_lock<std::mutex> lock_clients{m_clients_mutex};
        bool is_empty = m_clients.empty();
        for (const auto& ptr : m_clients)
        {
            std::scoped_lock<std::mutex> lock{m_to_delete_clients_mutex};
            m_to_delete_clients.push(ptr->get_id());
            m_to_delete_clients_cv.notify_one();
        }
        lock_clients.unlock();
        if (is_empty)
        {
            std::scoped_lock<std::mutex> lock{m_to_delete_clients_mutex};
            m_to_delete_clients.push("DummyClient");
            m_to_delete_clients_cv.notify_one();
        }
    }
}

void Server::accept()
{
    std::scoped_lock<std::mutex> client_lock{m_clients_mutex};
    std::string new_user_client_id = "Client_" + std::to_string(m_client_count);
    m_client_count++;
    m_clients.emplace_back(std::make_unique<Client>(
        new_user_client_id,
        std::bind(&Server::on_read_done, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4),
        std::bind(&Server::on_write_done, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4)
    ));
    m_acceptor.async_accept(m_clients.back()->get_socket(),
        [this, new_user_client_id](const asio::error_code &error)
        {
            handle_accept(error, new_user_client_id);
            accept();
        }
    );
}

}   // namespace kredis