#include "Server.hpp"
#include <spdlog/sinks/stdout_color_sinks.h>
#include "RespGenerator.hpp"
#include "RespErrors.hpp"

namespace kredis
{
void Server::write_to_client(std::string message, const std::string &client_id)
{
    auto client_ptr = m_client_manager.get_client(client_id);
    if (client_ptr)
    {
        client_ptr->write(message);
    }
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
        auto client_ptr = m_client_manager.get_client(client_id);
        if (!client_ptr)
        {
            return;
        }
        if (!error)
        {
            client_ptr->get_socket().set_option(asio::ip::tcp::no_delay(true));
            m_logger->debug("Waiting for read to complete for from [ClientId {}, Local endpoint address {}]",
                client_id,
                client_ptr->get_socket().local_endpoint().address().to_string());
            client_ptr->read();
        }
        else
        {
            m_client_manager.remove_client(client_id);
        }
    }
}

std::tuple<bool, std::size_t> Server::on_read_done(std::string_view message, asio::error_code error, std::size_t bytes_transferred, const std::string& client_id)
{
    if (error)
    {
        if (error == asio::error::connection_reset || error == asio::error::eof)
        {
            // Client is no longer connected, remove from clients list
            m_logger->warn("Client is no longer connected [ClientId {}]", client_id);
            m_client_manager.remove_client(client_id);
        }
        else
        {
            m_logger->warn("Error occured during read: {}", error.message());
        }
        return std::make_tuple(true, bytes_transferred);
    }
    // m_logger->info("Received message [Length={}] from [ClientId {}]:\n{}", message.length(), client_id, message);

    // Parse the message
    std::vector<RespParserResult> parse_results;
    if (!m_resp_parser.parse(message, parse_results))
    {
        m_logger->error("Failed to parse message from [ClientId {}]: {}", client_id, message);
        // Send error response
        RespError error{invalid_input_format};
        std::string error_response = m_resp_generator.generate(error);
        write_to_client(std::move(error_response), client_id);
        return std::make_tuple(true, parse_results.empty() ? 0 : 1);
    }

    std::shared_ptr<RespType> response_value;
    std::string response_message{};
    bool wait_for_next_read = false;
    std::size_t last_processed_index = 0;
    for (const auto& parse_result : parse_results)
    {
        //m_logger->debug("ParseResult: {} {} {}", static_cast<int>(parse_result.m_state), parse_result.m_parse_start_pos, parse_result.m_parse_end_pos);
        RespError error{invalid_input_format};
        if (parse_result.m_state == RespParserState::Invalid)
        {
            last_processed_index = parse_result.m_parse_end_pos;
            response_message += m_resp_generator.generate(error);
        }
        else if (parse_result.m_state == RespParserState::Incomplete)
        {
            wait_for_next_read = true;
            break;
        }
        else if (parse_result.m_state == RespParserState::Complete)
        {
            last_processed_index = parse_result.m_parse_end_pos;
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
                    response_message += m_resp_generator.generate(std::move(response_value));
                }
            }
        }
    }

    // Write back the response to client
    if (!response_message.empty())
    {
        write_to_client(std::move(response_message), client_id);
    }
    return std::make_tuple(!wait_for_next_read, last_processed_index);
}

Server::Server()
    : m_signals{m_io_context},
    m_acceptor{m_io_context},
    m_logger{spdlog::stdout_color_mt("RedisServer")},
    m_client_manager{m_logger},
    m_resp_parser{m_logger},
    m_resp_generator{m_logger},
    m_command_processor{m_logger}
{
    m_logger->set_level(spdlog::level::err);
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

    accept();
}

Server::~Server()
{
    if (!m_is_stopped)
    {
        stop();
    }
}

void Server::start()
{
    m_io_context.run();
}

void Server::stop()
{
    m_io_context.stop();
    m_is_stopped = true;
}

void Server::accept()
{
    auto client_ptr = m_client_manager.create_new_client(
        std::bind(&Server::on_read_done, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4)
    );
    m_acceptor.async_accept(client_ptr->get_socket(),
        [this, client_ptr](const asio::error_code &error)
        {
            handle_accept(error, client_ptr->get_id());
            accept();
        }
    );
}

}   // namespace kredis