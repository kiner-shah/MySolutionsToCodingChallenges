#pragma once

#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <spdlog/spdlog.h>
#include <thread>
#include <array>
#include <memory>
#include <deque>
#include <mutex>
#include "TcpBuffer.hpp"

namespace kredis
{
using ReadCallbackType = std::function<std::tuple<bool, std::size_t>(std::string_view, asio::error_code, std::size_t, const std::string&)>;

class Client : public std::enable_shared_from_this<Client>
{
    asio::ip::tcp::socket m_socket;

    std::array<unsigned char, 2048> m_read_buffer;
    // std::array<unsigned char, 2048> m_write_buffer;
    std::deque<std::string> m_write_queue;
    std::mutex m_write_queue_mutex;

    TcpBuffer m_input_buffer;

    ReadCallbackType m_on_read_done;

    std::string m_id;

    std::shared_ptr<spdlog::logger> m_logger;

    void handle_read(const asio::error_code& error, std::size_t bytes_transferred);
    void handle_write(const asio::error_code& error, std::size_t bytes_transferred);

public:
    Client(std::string id, asio::io_context& io_context, std::shared_ptr<spdlog::logger> logger, ReadCallbackType on_read);
    ~Client();
    asio::ip::tcp::socket& get_socket();
    std::string get_id() const;
    void read();
    void write(std::string payload);
};

using ClientPtr = std::shared_ptr<Client>;

}   // namespace kredis