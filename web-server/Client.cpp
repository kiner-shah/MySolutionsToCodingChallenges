#include "Client.hpp"
#include <asio/write.hpp>

namespace kweb_server
{
Client::Client(const std::string &id, asio::io_context &io_context, ClientReadCallbackType on_read, ClientWriteCallbackType on_write)
    : m_socket(io_context),
      m_client_id(id),
      m_read_buffer(2048, 0),
      m_write_buffer(2048, 0),
      m_on_read(on_read),
      m_on_write(on_write)
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
            // TODO: log message?
        }
        m_socket.close();
        if (error)
        {
            // TODO: log message?
        }
    }
}

asio::ip::tcp::socket &Client::get_socket()
{
    return m_socket;
}

std::string Client::get_id() const
{
    return m_client_id;
}

void Client::read()
{
    m_socket.async_read_some(
        asio::buffer(m_read_buffer),
        [self = shared_from_this()](const asio::error_code& error, std::size_t bytes_transferred)
        {
            if (error)
            {
                self->m_on_read({}, error, 0, self->m_client_id);
                return;
            }

            self->m_request_buffer.append(
                reinterpret_cast<const char*>(self->m_read_buffer.data()),
                bytes_transferred
            );

            std::fill(self->m_read_buffer.begin(), self->m_read_buffer.end(), 0);

            std::size_t header_end = self->m_request_buffer.find("\r\n\r\n");
            if (header_end == std::string::npos)
            {
                // Header not fully received yet, continue reading
                self->read();
                return;
            }

            self->m_on_read(
                self->m_request_buffer,
                error,
                header_end + 4,
                self->m_client_id
            );
            self->m_request_buffer.erase(0, header_end + 4);
        }
    );
}

void Client::write(const std::string& payload, std::size_t payload_length)
{
    m_write_buffer.assign(payload.begin(), payload.begin() + payload_length);
    asio::async_write(
        m_socket,
        asio::buffer(m_write_buffer, payload_length),
        [self = shared_from_this()](const asio::error_code& error, std::size_t bytes_transferred)
        {
            self->m_on_write(self->m_write_buffer, error, bytes_transferred, self->m_client_id);
        }
    );
}
} // namespace kweb_server
