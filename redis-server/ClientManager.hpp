#pragma once

#include <vector>
#include <memory>
#include <unordered_map>
#include <thread>
#include <functional>
#include <shared_mutex>
#include <asio/io_context.hpp>
#include <spdlog/spdlog.h>
#include "Client.hpp"

namespace kredis
{
class ClientManager
{
    using ClientPtr = std::shared_ptr<Client>;
    using ReadCallbackType = std::function<bool(const std::string&, asio::error_code, std::size_t, const std::string&)>;
    using WriteCallbackType = std::function<void(const std::array<unsigned char, 2048>&, asio::error_code, std::size_t, const std::string&)>;

    std::unordered_map<std::string, ClientPtr> m_clients;
    asio::io_context m_io_context;
    asio::executor_work_guard<asio::io_context::executor_type> m_work_guard;
    std::vector<std::thread> m_threads;
    std::shared_ptr<spdlog::logger> m_logger;
    std::uint64_t m_client_counter;

    std::shared_mutex m_clients_mutex;
public:
    ClientManager(std::shared_ptr<spdlog::logger> logger);
    ~ClientManager();
    ClientPtr create_new_client(ReadCallbackType on_read, WriteCallbackType on_write);
    ClientPtr get_client(const std::string& id);
    void remove_client(const std::string& id);
};
}   // namespace kredis