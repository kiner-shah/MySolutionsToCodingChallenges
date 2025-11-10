#include "Client.hpp"

namespace kredis
{
void Client::handle_read(const asio::error_code &error, std::size_t bytes_transferred)
{
    if (!m_input_buffer.empty())
    {
        auto ec = m_input_buffer.append(reinterpret_cast<char*>(m_read_buffer.data()), bytes_transferred);
        if (ec == std::errc{})
        {
            auto [_, processed_bytes] = m_on_read_done(m_input_buffer.view(), error, bytes_transferred, m_id);
            m_input_buffer.consume(processed_bytes);
        }
    }
    else
    {
        std::string_view data_view{reinterpret_cast<char*>(m_read_buffer.data()), bytes_transferred};
        auto [_, processed_bytes] = m_on_read_done(data_view, error, bytes_transferred, m_id);
        if (processed_bytes < bytes_transferred)
        {
            m_input_buffer.append(reinterpret_cast<char*>(m_read_buffer.data()) + processed_bytes, bytes_transferred - processed_bytes);
        }
    }

    if (error != asio::error::connection_reset && error != asio::error::eof)
    {
        read();
    }
}

void Client::handle_write(const asio::error_code &error, std::size_t bytes_transferred)
{
    if (error)
    {
        //m_logger->error("Error occured during write: {}", error.message());
    }

    std::unique_lock<std::mutex> lock{m_write_queue_mutex};
    m_write_queue.pop_front();

    if (!m_write_queue.empty())
    {
        m_socket.async_write_some(
            asio::buffer(m_write_queue.front()),
            [self = shared_from_this()](const asio::error_code& error, std::size_t bytes_transferred)
            {
                self->handle_write(error, bytes_transferred);
            }
        );
    }
}

Client::Client(std::string id, asio::io_context& io_context, std::shared_ptr<spdlog::logger> logger, ReadCallbackType on_read)
    : m_socket{io_context},
    m_on_read_done{std::move(on_read)}, m_id{std::move(id)}, m_logger{std::move(logger)}
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

void Client::write(std::string payload)
{
    std::unique_lock<std::mutex> lock{m_write_queue_mutex};
    bool write_in_progress = !m_write_queue.empty();
    m_write_queue.push_back(std::move(payload));

    if (!write_in_progress)
    {
        m_socket.async_write_some(
            asio::buffer(m_write_queue.front()),
            [self = shared_from_this()](const asio::error_code& error, std::size_t bytes_transferred)
            {
                self->handle_write(error, bytes_transferred);
            }
        );
    }
}
}   // namespace kredis