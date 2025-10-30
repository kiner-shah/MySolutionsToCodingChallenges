#include "Client.hpp"

namespace kredis
{
void Client::handle_read(const asio::error_code &error, std::size_t bytes_transferred)
{
    m_input_message.append(std::string{m_read_buffer.begin(), m_read_buffer.begin() + bytes_transferred});
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

Client::Client(std::string id, std::shared_ptr<spdlog::logger> logger, ReadCallbackType on_read, WriteCallbackType on_write)
    : m_work_guard{asio::make_work_guard(m_io_context)}, m_socket{m_io_context},
    m_on_read_done{std::move(on_read)}, m_on_write_done{std::move(on_write)}, m_id{std::move(id)}, m_logger{std::move(logger)}
{
    m_io_context_thread = std::thread([this]() { m_io_context.run(); });
}

Client::~Client()
{
    if (m_socket.is_open())
    {
        m_socket.cancel();
    }
    m_work_guard.reset();
    while (!m_io_context.stopped())
    {
        m_io_context.stop();
    }
    m_io_context_thread.join();
    if (m_socket.is_open())
    {
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
    m_socket.async_read_some(asio::buffer(m_read_buffer), std::bind(&Client::handle_read, this, std::placeholders::_1, std::placeholders::_2));
}

void Client::write(const std::array<unsigned char, 2048> &payload, std::size_t length)
{
    std::copy(payload.begin(), payload.end(), m_write_buffer.begin());
    m_socket.async_write_some(
        asio::buffer(m_write_buffer, length),
        std::bind(&Client::handle_write, this, std::placeholders::_1, std::placeholders::_2)
    );
}
}