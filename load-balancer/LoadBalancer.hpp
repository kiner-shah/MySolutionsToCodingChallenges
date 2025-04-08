#pragma once

#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/signal_set.hpp>
#include <string_view>
#include <vector>
#include <mutex>
#include "UserClient.hpp"
#include "Client.hpp"

namespace kload_balancer
{
class LoadBalancer
{
    using UserClientPtr = std::unique_ptr<UserClient>;
    using ClientPtr = std::unique_ptr<Client>;

    class ServerDetails
    {
        std::string_view m_ip;
        std::string_view m_port;
        bool m_is_available;
    public:
        ServerDetails(std::string_view ip, std::string_view port);
        bool is_available() const;
        std::string_view get_ip() const;
        std::string_view get_port() const;
        void set_available(bool status);
    };

    asio::io_context m_io_context;
    asio::signal_set m_signals;
    std::vector<UserClientPtr> m_user_clients;
    std::mutex m_user_clients_mutex;
    std::vector<ClientPtr> m_clients;
    std::mutex m_clients_mutex;
    std::vector<ServerDetails> m_servers;
    asio::ip::tcp::acceptor m_acceptor;
    bool m_is_stopped = false;
    std::size_t m_last_server_index;
    unsigned short m_port;

    std::int_least64_t m_user_client_count;

    void handle_accept(const asio::error_code& error, std::string user_client_id);
    bool get_next_available_server(std::size_t& index);
    void on_client_read_done(std::array<unsigned char, 2048>, asio::error_code, std::size_t, std::string);
    void on_client_write_done(std::array<unsigned char, 2048>, asio::error_code, std::size_t, std::string);
    void on_server_read_done(std::array<unsigned char, 2048>, asio::error_code, std::size_t, std::string, std::string);
    void on_server_write_done(std::array<unsigned char, 2048>, asio::error_code, std::size_t, std::string, std::string);
public:
    LoadBalancer(unsigned short port);
    ~LoadBalancer();
    void start();
    void stop();
    void add_server(std::string_view ip_address, std::string_view port);
    void accept();
};
}   // namespace kload_balancer