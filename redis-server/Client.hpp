#pragma once

#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <spdlog/spdlog.h>
#include <thread>
#include <array>
#include <memory>

namespace kredis
{
class Client
{
    using ReadCallbackType = std::function<bool(const std::string&, asio::error_code, std::size_t, const std::string&)>;
    using WriteCallbackType = std::function<void(const std::array<unsigned char, 2048>&, asio::error_code, std::size_t, const std::string&)>;

    asio::io_context m_io_context;
    asio::executor_work_guard<asio::io_context::executor_type> m_work_guard;
    asio::ip::tcp::socket m_socket;
    std::thread m_io_context_thread;

    std::array<unsigned char, 2048> m_read_buffer;
    std::array<unsigned char, 2048> m_write_buffer;
    std::string m_input_message;

    ReadCallbackType m_on_read_done;
    WriteCallbackType m_on_write_done;

    std::string m_id;

    std::shared_ptr<spdlog::logger> m_logger;

    void handle_read(const asio::error_code& error, std::size_t bytes_transferred);
    void handle_write(const asio::error_code& error, std::size_t bytes_transferred);

public:
    Client(std::string id, std::shared_ptr<spdlog::logger> logger, ReadCallbackType on_read, WriteCallbackType on_write);
    ~Client();
    asio::ip::tcp::socket& get_socket();
    std::string get_id() const;
    void read();
    void write(const std::array<unsigned char, 2048>& payload, std::size_t length);
};
}   // namespace kredis