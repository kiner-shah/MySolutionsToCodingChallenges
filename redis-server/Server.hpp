#pragma once

#include <asio/io_context.hpp>
#include <asio/signal_set.hpp>
#include <asio/ip/tcp.hpp>
#include <spdlog/spdlog.h>
#include <memory>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
// #include "Client.hpp"
#include "ClientManager.hpp"
#include "RespParser.hpp"
#include "RespGenerator.hpp"
#include "CommandProcessor.hpp"

namespace kredis
{
class Server
{
    asio::io_context m_io_context;
    asio::signal_set m_signals;
    asio::ip::tcp::acceptor m_acceptor;
    std::shared_ptr<spdlog::logger> m_logger;
    ClientManager m_client_manager;
    std::atomic_bool m_is_stopped = false;

    RespParser m_resp_parser;
    RespGenerator m_resp_generator;
    CommandProcessor m_command_processor;

    void write_to_client(std::string&& message, const std::string& client_id);
    void handle_accept(const asio::error_code& error, std::string client_id);
    std::tuple<bool, std::size_t> on_read_done(std::string_view, asio::error_code, std::size_t, const std::string&);

public:
    Server();
    ~Server();
    void start();
    void stop();
    void accept();
};
}   // namespace kredis