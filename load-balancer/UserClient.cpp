#include "UserClient.hpp"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <iostream>

namespace kload_balancer
{
UserClient::UserClient(std::string id, asio::io_context& io_context, UserClientCallbackType on_read, UserClientCallbackType on_write)
    : m_socket{io_context},
    m_on_read_complete{on_read}, m_on_write_complete{on_write}, m_id{std::move(id)}
{
}

UserClient::~UserClient()
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
        m_socket.close(error);
        if (error)
        {
            // TODO: log message?
        }
    }
}

asio::ip::tcp::socket &UserClient::get_socket()
{
    return m_socket;
}

std::string UserClient::get_id() const
{
    return m_id;
}

void UserClient::read()
{
    m_socket.async_read_some(asio::buffer(m_read_buffer),
        [self = shared_from_this()](const asio::error_code &error, std::size_t bytes_transferred)
        {
            self->m_on_read_complete(self->m_read_buffer, error, bytes_transferred, self->m_id);
        }
    );
}

void UserClient::write(const std::array<unsigned char, 2048> &payload)
{
    std::copy(payload.begin(), payload.end(), m_write_buffer.begin());
    m_socket.async_write_some(asio::buffer(m_write_buffer),
        [self = shared_from_this()](const asio::error_code &error, std::size_t bytes_transferred)
        {
            self->m_on_write_complete(self->m_write_buffer, error, bytes_transferred, self->m_id);
        }
    );
}

}   // namespace kload_balancer