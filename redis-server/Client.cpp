#include "Client.hpp"

namespace kredis
{
void Client::handle_read(const asio::error_code &error, std::size_t bytes_transferred)
{
    if (m_on_read_done)
    {
        m_on_read_done(m_read_buffer, error, bytes_transferred, m_id);
    }
}

void Client::handle_write(const asio::error_code &error, std::size_t bytes_transferred)
{
    if (m_on_write_done)
    {
        m_on_write_done(m_write_buffer, error, bytes_transferred, m_id);
    }
}

Client::Client(std::string id, CallbackType on_read, CallbackType on_write)
    : m_work_guard{asio::make_work_guard(m_io_context)}, m_socket{m_io_context},
    m_on_read_done{std::move(on_read)}, m_on_write_done{std::move(on_write)}, m_id{std::move(id)}
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
        m_socket.shutdown(asio::socket_base::shutdown_both);
        m_socket.close();
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

void Client::write(const std::array<unsigned char, 2048> &payload)
{
    m_socket.async_write_some(asio::buffer(payload), std::bind(&Client::handle_write, this, std::placeholders::_1, std::placeholders::_2));
}
}