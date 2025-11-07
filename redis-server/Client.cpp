#include "Client.hpp"

namespace kredis
{
void Client::handle_read(const asio::error_code &error, std::size_t bytes_transferred)
{
    m_input_message.append(m_read_buffer.begin(), m_read_buffer.begin() + bytes_transferred);
    bool status = m_on_read_done(m_input_message, error, bytes_transferred, m_id);
    if (status)
    {
        m_input_message.clear();
    }
    if (error != asio::error::connection_reset && error != asio::error::eof)
    {
        read();
    }
}

void Client::handle_write(const asio::error_code &error, std::size_t bytes_transferred)
{
    if (m_on_write_done)
    {
        m_on_write_done(m_write_buffer, error, bytes_transferred, m_id);
    }
}

Client::Client(std::string id, asio::io_context& io_context, std::shared_ptr<spdlog::logger> logger, ReadCallbackType on_read, WriteCallbackType on_write)
    : m_socket{io_context},
    m_on_read_done{std::move(on_read)}, m_on_write_done{std::move(on_write)}, m_id{std::move(id)}, m_logger{std::move(logger)}
{
}

Client::~Client()
{
    if (m_socket.is_open())
    {
        m_socket.cancel();

        asio::error_code error;
        // Attempt shutdown only if the socket is connected
        m_socket.shutdown(asio::socket_base::shutdown_both, error);
        if (error)
        {
            m_logger->warn("Shutdown failed: {}", error.message());
        }

        m_socket.close(error);
        if (error)
        {
            m_logger->warn("Close failed: {}", error.message());
        }
    }
}
asio::ip::tcp::socket &Client::get_socket()
{
    return m_socket;
}

std::string Client::get_id() const
{
    return m_id;
}

void Client::read()
{
    m_socket.async_read_some(
        asio::buffer(m_read_buffer),
        [self = shared_from_this()](const asio::error_code& error, std::size_t bytes_transferred)
        {
            self->handle_read(error, bytes_transferred);
        }
    );
}

void Client::write(const std::string& payload)
{
    std::copy(payload.begin(), payload.end(), m_write_buffer.begin());
    m_socket.async_write_some(
        asio::buffer(m_write_buffer, payload.size()),
        [self = shared_from_this()](const asio::error_code& error, std::size_t bytes_transferred)
        {
            self->handle_write(error, bytes_transferred);
        }
    );
}
}