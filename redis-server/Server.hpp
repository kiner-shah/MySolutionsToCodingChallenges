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
#include "Client.hpp"
#include "RespParser.hpp"
#include "RespGenerator.hpp"
#include "CommandProcessor.hpp"

namespace kredis
{
class Server
{
    using ClientPtr = std::unique_ptr<Client>;

    asio::io_context m_io_context;
    asio::signal_set m_signals;
    asio::ip::tcp::acceptor m_acceptor;
    std::shared_ptr<spdlog::logger> m_logger;
    std::vector<ClientPtr> m_clients;
    std::mutex m_clients_mutex;
    std::int_least64_t m_client_count;
    std::atomic_bool m_is_stopped = false;

    std::queue<std::string> m_to_delete_clients;
    std::mutex m_to_delete_clients_mutex;
    std::condition_variable m_to_delete_clients_cv;
    std::thread m_unused_clients_deleter;
    RespParser m_resp_parser;
    RespGenerator m_resp_generator;
    CommandProcessor m_command_processor;

    void remove_client(const std::string& client_id);
    void handle_accept(const asio::error_code& error, std::string client_id);
    bool on_read_done(const std::string&, asio::error_code, std::size_t, const std::string&);
    void on_write_done(const std::array<unsigned char, 2048>&, asio::error_code, std::size_t, const std::string&);
public:
    Server();
    ~Server();
    void start();
    void stop();
    void accept();
};
}   // namespace kredis