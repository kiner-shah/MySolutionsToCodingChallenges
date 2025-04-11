#include "UserClient.hpp"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <iostream>

namespace kload_balancer
{
UserClient::UserClient(std::string id, CallbackType on_read, CallbackType on_write)
    : m_work_guard{asio::make_work_guard(m_io_context)}, m_socket{m_io_context},
    m_on_read_complete{on_read}, m_on_write_complete{on_write}, m_id{std::move(id)}
{
    m_io_context_thread = std::thread([this]() { m_io_context.run(); });
}

UserClient::~UserClient()
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
        m_socket.shutdown(asio::socket_base::shutdown_both);
        m_socket.close();
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
        [this](const asio::error_code &error, std::size_t bytes_transferred)
        {
            handle_read(error, bytes_transferred);
        }
    );
}

void UserClient::write(const std::array<unsigned char, 2048> &payload)
{
    std::copy(payload.begin(), payload.end(), m_write_buffer.begin());
    m_socket.async_write_some(asio::buffer(m_write_buffer),
        [this](const asio::error_code &error, std::size_t bytes_transferred)
        {
            handle_write(error, bytes_transferred);
        }
    );
}

void UserClient::handle_read(const asio::error_code &error, std::size_t bytes_transferred)
{
    m_on_read_complete(m_read_buffer, error, bytes_transferred, m_id);
}

void UserClient::handle_write(const asio::error_code &error, std::size_t bytes_transferred)
{
    m_on_write_complete(m_write_buffer, error, bytes_transferred, m_id);
}

}   // namespace kload_balancer