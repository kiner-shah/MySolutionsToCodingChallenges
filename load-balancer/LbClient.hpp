#pragma once

#include <asio/ip/tcp.hpp>
#include <memory>
#include <functional>
#include <array>

namespace kload_balancer
{
using LbClientCallbackType = std::function<void(std::array<unsigned char, 2048>, asio::error_code, std::size_t, std::string, std::string)>;

/// @brief  This struct handles connection to a backend server. So basically it acts like a client to backend server.
class LbClient : public std::enable_shared_from_this<LbClient>
{   
    asio::ip::tcp::endpoint m_server_endpoint;
    asio::ip::tcp::socket m_socket;

    std::array<unsigned char, 2048> m_read_buffer;
    std::array<unsigned char, 2048> m_write_buffer;

    LbClientCallbackType m_on_read_complete;
    LbClientCallbackType m_on_write_complete;

    std::string m_id;
    std::string m_ip_port;

    bool m_is_for_health_check;

public:
    LbClient(asio::io_context& io_context, std::string_view ip_address, std::string_view port, LbClientCallbackType on_read, LbClientCallbackType on_write, bool is_for_health_check);
    ~LbClient();

    void read(std::string user_client_id);
    void write(const std::array<unsigned char, 2048>& data, std::string user_client_id);
    std::string get_id() const;
    std::string get_ip_port() const;
};

using LbClientPtr = std::shared_ptr<LbClient>;
}   // namespace kload_balancer