#pragma once

#include "LbClient.hpp"
#include <unordered_map>
#include <shared_mutex>
#include <asio/io_context.hpp>

namespace kload_balancer
{
class LbClientManager
{
    std::unordered_map<std::string, LbClientPtr> m_lb_clients;
    std::shared_mutex m_lb_clients_mutex;
public:
    LbClientPtr create_new_lb_client(asio::io_context& io_context, std::string_view ip_address, std::string_view port, LbClientCallbackType on_read, LbClientCallbackType on_write, bool is_for_health_check = false);
    LbClientPtr get_lb_client(const std::string& ip_port);
    void remove_lb_client(const std::string& ip_port);
};
}   // namespace kload_balancer