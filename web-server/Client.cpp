#include "Client.hpp"
#include <asio/write.hpp>

namespace kweb_server
{
Client::Client(const std::string &id, asio::io_context &io_context, ClientCallbackType on_read, ClientCallbackType on_write)
    : m_socket(io_context),
      m_client_id(id),
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
            self->m_on_read(self->m_read_buffer, error, bytes_transferred, self->m_client_id);
        }
    );
}

void Client::write(const std::array<unsigned char, 2048> &payload)
{
    asio::async_write(
        m_socket,
        asio::buffer(payload),
        [self = shared_from_this()](const asio::error_code& error, std::size_t bytes_transferred)
        {
            self->m_on_write(self->m_write_buffer, error, bytes_transferred, self->m_client_id);
        }
    );
}
} // namespace kweb_server
