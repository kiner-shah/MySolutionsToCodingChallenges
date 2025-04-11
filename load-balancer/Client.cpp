#include "Client.hpp"
#include <asio/connect.hpp>

namespace kload_balancer
{
Client::Client(std::string_view ip_address, std::string_view port, CallbackType on_read, CallbackType on_write)
    : m_work_guard{asio::make_work_guard(m_io_context)}, m_socket{m_io_context}, m_on_read_complete{on_read}, m_on_write_complete{on_write}
{
    asio::ip::tcp::resolver server_resolver{m_io_context};
    auto endpoints = server_resolver.resolve(ip_address, port);
    m_server_endpoint = asio::connect(m_socket, endpoints);

    asio::ip::address_v4 endpoint_ip = m_server_endpoint.address().to_v4();
    asio::ip::port_type endpoint_port = m_server_endpoint.port();

    m_ip_port = endpoint_ip.to_string() + ':' + std::to_string(endpoint_port);

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

void Client::read(std::string user_client_id)
{
    std::fill(m_read_buffer.begin(), m_read_buffer.end(), 0x0);
    m_socket.async_read_some(asio::buffer(m_read_buffer),
        [this, user_client_id](const asio::error_code &error, std::size_t bytes_transferred)
        {
            handle_read(error, bytes_transferred, std::move(user_client_id));
        }
    );
}

void Client::write(const std::array<unsigned char, 2048> &data, std::string user_client_id)
{
    std::fill(m_write_buffer.begin(), m_write_buffer.end(), 0x0);
    std::copy(data.begin(), data.end(), m_write_buffer.begin());
    // m_socket.async_write_some(asio::buffer(m_write_buffer), handle_write);
    m_socket.async_write_some(asio::buffer(m_write_buffer),
        [this, user_client_id](const asio::error_code &error, std::size_t bytes_transferred)
        {
            handle_write(error, bytes_transferred, std::move(user_client_id));
        }
    );
}

std::string Client::get_ip_port() const
{
    return m_ip_port;
}

void Client::handle_read(const asio::error_code &error, std::size_t bytes_transferred, std::string user_client_id)
{
    m_on_read_complete(m_read_buffer, error, bytes_transferred, user_client_id, get_ip_port());
}

void Client::handle_write(const asio::error_code &error, std::size_t bytes_transferred, std::string user_client_id)
{
    m_on_write_complete(m_write_buffer, error, bytes_transferred, user_client_id, get_ip_port());
}
}   // namespace kload_balancer