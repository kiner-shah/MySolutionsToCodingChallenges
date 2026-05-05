#pragma once

#include "ClientManager.hpp"
#include "HttpMethods.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "ThreadPool.hpp"
#include <array>
#include <atomic>
#include <asio/ip/tcp.hpp>
#include <asio/signal_set.hpp>
#include <functional>
#include <memory>
#include <string>
#include <spdlog/spdlog.h>
#include <unordered_map>

namespace kweb_server
{
class WebServer
{
    std::shared_ptr<ThreadPool> m_thread_pool;
    std::shared_ptr<spdlog::logger> m_logger;
    std::unique_ptr<ClientManager> m_client_manager;
    asio::ip::tcp::resolver m_endpoint_resolver;
    asio::signal_set m_signals;
    asio::ip::tcp::acceptor m_acceptor;
    std::atomic_bool m_is_stopped = false;
    std::unordered_map<HttpRequest, std::function<HttpResponse(const HttpRequest&)>> m_handlers;
    std::string m_www_root;

    void handle_client_read(std::string_view, const asio::error_code&, std::size_t, const std::string&);
    void handle_client_write(const std::vector<unsigned char>&, const asio::error_code&, std::size_t, const std::string&);
    void handle_accept(const asio::error_code& error, const std::string& client_id);
public:
    WebServer(unsigned short port, const std::string& www_root);
    ~WebServer();
    void accept();
    void start();
    void stop();
    void register_handler(HttpMethod method, const std::string& path, std::function<HttpResponse(const HttpRequest&)> handler);
};
}   // namespace kweb_server