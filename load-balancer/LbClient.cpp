#include "LbClient.hpp"
#include <asio/connect.hpp>

namespace kload_balancer
{
LbClient::LbClient(
    asio::io_context& io_context,
    std::string_view ip_address,
    std::string_view port,
    LbClientCallbackType on_read,
    LbClientCallbackType on_write,
    bool is_for_health_check)
    : m_socket{io_context},
    m_on_read_complete{on_read},
    m_on_write_complete{on_write},
    m_is_for_health_check{is_for_health_check}
{
    asio::ip::tcp::resolver server_resolver{io_context};
    auto endpoints = server_resolver.resolve(ip_address, port);
    m_server_endpoint = asio::connect(m_socket, endpoints);

    asio::ip::address_v4 endpoint_ip = m_server_endpoint.address().to_v4();
    asio::ip::port_type endpoint_port = m_server_endpoint.port();

    m_ip_port = endpoint_ip.to_string() + ':' + std::to_string(endpoint_port);

    m_id = endpoint_ip.to_string() + '_' + std::to_string(endpoint_port);
    m_id += m_is_for_health_check ? "_H" : "_NH";
}

LbClient::~LbClient()
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

void LbClient::read(std::string user_client_id)
{
    std::fill(m_read_buffer.begin(), m_read_buffer.end(), 0x0);
    m_socket.async_read_some(asio::buffer(m_read_buffer),
        [self = shared_from_this(), user_client_id](const asio::error_code &error, std::size_t bytes_transferred)
        {
            self->m_on_read_complete(self->m_read_buffer, error, bytes_transferred, user_client_id, self->get_id());
        }
    );
}

void LbClient::write(const std::array<unsigned char, 2048> &data, std::string user_client_id)
{
    std::fill(m_write_buffer.begin(), m_write_buffer.end(), 0x0);
    std::copy(data.begin(), data.end(), m_write_buffer.begin());
    // m_socket.async_write_some(asio::buffer(m_write_buffer), handle_write);
    m_socket.async_write_some(asio::buffer(m_write_buffer),
        [self = shared_from_this(), user_client_id](const asio::error_code &error, std::size_t bytes_transferred)
        {
            self->m_on_write_complete(self->m_write_buffer, error, bytes_transferred, user_client_id, self->get_id());
        }
    );
}

std::string LbClient::get_id() const
{
    return m_id;
}

std::string LbClient::get_ip_port() const
{
    return m_ip_port;
}

}   // namespace kload_balancer