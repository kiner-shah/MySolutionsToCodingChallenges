#include "Server.hpp"
#include <spdlog/sinks/stdout_color_sinks.h>
#include "RespGenerator.hpp"
#include "RespErrors.hpp"

namespace kredis
{
void Server::remove_client(const std::string& client_id)
{
    std::scoped_lock<std::mutex> lock{m_to_delete_clients_mutex};
    m_to_delete_clients.push(client_id);
    m_to_delete_clients_cv.notify_one();
}

void Server::handle_accept(const asio::error_code& error, std::string client_id)
{
    // Check whether the server was stopped by a signal before this
    // completion handler had a chance to run.
    if (!m_acceptor.is_open())
    {
        return;
    }

    m_logger->info("New connection request [ClientId {}]", client_id);
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

bool Server::on_read_done(const std::string& message, asio::error_code error, std::size_t bytes_transferred, const std::string& client_id)
{
    auto find_client = [&client_id](const ClientPtr& client) { return client_id == client->get_id(); };
    auto write_to_client = [&client_id, &find_client, this](const std::string& message)
    {
        std::scoped_lock<std::mutex> lock{m_clients_mutex};
        auto it = std::find_if(m_clients.begin(), m_clients.end(), find_client);
        if (it != m_clients.end())
        {
            std::array<unsigned char, 2048> buffer;
            std::copy(message.begin(), message.end(), buffer.begin());
            (*it)->write(buffer, message.length());
        }
    };

    if (error)
    {
        if (error == asio::error::connection_reset || error == asio::error::eof)
        {
            // Client is no longer connected, remove from clients list
            m_logger->warn("Client is no longer connected [ClientId {}]", client_id);
            remove_client(client_id);
        }
        else
        {
            m_logger->warn("Error occured during read: {}", error.message());
        }
        return true;
    }
    m_logger->info("Received message [Length={}] from [ClientId {}]:\n{}", message.length(), client_id, message);

    // Parse the message
    std::vector<RespParserResult> parse_results;
    if (!m_resp_parser.parse(message, parse_results))
    {
        m_logger->error("Failed to parse message from [ClientId {}]: {}", client_id, message);
        // Send error response (RespError)
        RespError error{invalid_input_format};
        std::string error_response = m_resp_generator.generate(error);
        write_to_client(error_response);
        return true;
    }

    RespType response_value;
    std::string response_message{};
    for (const auto& parse_result : parse_results)
    {
        RespError error{invalid_input_format};
        if (parse_result.m_state == RespParserState::Invalid)
        {
            response_message += m_resp_generator.generate(error);
        }
        else if (parse_result.m_state == RespParserState::Incomplete)
        {
            // return from this function, and wait for next read
            return false;
        }
        else if (parse_result.m_state == RespParserState::Complete)
        {
            if (!parse_result.m_type.has_value())
            {
                response_message = m_resp_generator.generate(RespError{internal_error});
            }
            else
            {
                // Parse the command and perform respectful operation
                if (!m_command_processor.process(*parse_result.m_type, response_value))
                {
                    response_message += m_resp_generator.generate(error);
                }
                else
                {
                    response_message += m_resp_generator.generate(response_value);
                }
            }
        }
    }

    // Write back the response to client
    write_to_client(response_message);
    return true;
}

void Server::on_write_done(const std::array<unsigned char, 2048>& write_data, asio::error_code error, std::size_t bytes_transferred, const std::string& client_id)
{
    if (error)
    {
        m_logger->warn("Error occured during write: {}", error.message());
    }
    m_logger->debug("Sent message [Length={}] to [ClientId {}]:\n{}", bytes_transferred, client_id, std::string{write_data.begin(), write_data.begin() + bytes_transferred});
    //remove_client(client_id);
}

Server::Server()
    : m_signals{m_io_context}, m_acceptor{m_io_context}, m_logger{spdlog::stdout_color_mt("RedisServer")},
    m_client_count{0}, m_resp_parser{m_logger}, m_resp_generator{m_logger}, m_command_processor{m_logger}
{
    m_logger->set_level(spdlog::level::warn);
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
    m_io_context.stop();
    {
        std::unique_lock<std::mutex> lock_clients{m_clients_mutex};
        bool is_empty = m_clients.empty();
        for (const auto& ptr : m_clients)
        {
            m_logger->debug("Stopping client [ClientId {}]", ptr->get_id());
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
    m_is_stopped = true;
}

void Server::accept()
{
    std::scoped_lock<std::mutex> client_lock{m_clients_mutex};
    std::string new_user_client_id = "Client_" + std::to_string(m_client_count);
    m_client_count++;
    m_clients.emplace_back(std::make_unique<Client>(
        new_user_client_id,
        m_logger,
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