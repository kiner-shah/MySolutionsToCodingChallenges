#pragma once

#include <asio/io_context.hpp>
#include <asio/executor_work_guard.hpp>
#include <asio/ip/tcp.hpp>
#include <thread>

namespace kload_balancer
{
class UserClient
{
    using CallbackType = std::function<void(std::array<unsigned char, 2048>, asio::error_code, std::size_t, std::string)>;

    asio::io_context m_io_context;
    asio::executor_work_guard<asio::io_context::executor_type> m_work_guard;
    asio::ip::tcp::socket m_socket;
    std::thread m_io_context_thread;

    std::array<unsigned char, 2048> m_read_buffer;
    std::array<unsigned char, 2048> m_write_buffer;
    
    CallbackType m_on_read_complete;
    CallbackType m_on_write_complete;

    std::string m_id;

    void handle_read(const asio::error_code& error, std::size_t bytes_transferred);
    void handle_write(const asio::error_code& error, std::size_t bytes_transferred);

public:
    UserClient(std::string id, CallbackType on_read, CallbackType on_write);
    ~UserClient();
    asio::ip::tcp::socket& get_socket();
    std::string get_id() const;
    void read();
    void write(const std::array<unsigned char, 2048>& payload);
};
}   // namespace kload_balancer