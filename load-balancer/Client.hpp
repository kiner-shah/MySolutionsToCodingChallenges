#pragma once

#include <asio/io_context.hpp>
#include <asio/executor_work_guard.hpp>
#include <asio/ip/tcp.hpp>
#include <thread>

namespace kload_balancer
{
/// @brief  This struct handles connection to a backend server. So basically it acts like a client to backend server.
class Client
{
    using CallbackType = std::function<void(std::array<unsigned char, 2048>, asio::error_code, std::size_t, std::string, std::string)>;

    asio::io_context m_io_context;
    asio::executor_work_guard<asio::io_context::executor_type> m_work_guard;
    std::thread m_io_context_thread;
    
    asio::ip::tcp::endpoint m_server_endpoint;
    asio::ip::tcp::socket m_socket;

    std::array<unsigned char, 2048> m_read_buffer;
    std::array<unsigned char, 2048> m_write_buffer;

    CallbackType m_on_read_complete;
    CallbackType m_on_write_complete;

    std::string m_ip_port;

    void handle_read(const asio::error_code& error, std::size_t bytes_transferred, std::string user_client_id);
    void handle_write(const asio::error_code& error, std::size_t bytes_transferred, std::string user_client_id);

public:
    Client(std::string_view ip_address, std::string_view port, CallbackType on_read, CallbackType on_write);
    ~Client();

    void read(std::string user_client_id);
    void write(const std::array<unsigned char, 2048>& data, std::string user_client_id);
    std::string get_ip_port() const;
};
}   // namespace kload_balancer