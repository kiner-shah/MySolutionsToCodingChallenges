#pragma once

#include <asio/ip/tcp.hpp>
#include <thread>
#include <memory>
#include <functional>
#include <array>

namespace kload_balancer
{
using UserClientCallbackType = std::function<void(std::array<unsigned char, 2048>, asio::error_code, std::size_t, std::string)>;

class UserClient : public std::enable_shared_from_this<UserClient>
{
    asio::ip::tcp::socket m_socket;

    std::array<unsigned char, 2048> m_read_buffer;
    std::array<unsigned char, 2048> m_write_buffer;
    
    UserClientCallbackType m_on_read_complete;
    UserClientCallbackType m_on_write_complete;

    std::string m_id;

public:
    UserClient(std::string id, asio::io_context& io_context, UserClientCallbackType on_read, UserClientCallbackType on_write);
    ~UserClient();
    asio::ip::tcp::socket& get_socket();
    std::string get_id() const;
    void read();
    void write(const std::array<unsigned char, 2048>& payload);
};

using UserClientPtr = std::shared_ptr<UserClient>;
}   // namespace kload_balancer