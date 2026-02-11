#include "LbClientManager.hpp"
#include <mutex>

namespace kload_balancer
{
LbClientPtr LbClientManager::create_new_lb_client(
    asio::io_context &io_context,
    std::string_view ip_address,
    std::string_view port,
    LbClientCallbackType on_read,
    LbClientCallbackType on_write,
    LbClientConnectCallbackType on_connect,
    bool is_for_health_check)
{
    auto lb_client = std::make_shared<LbClient>(
        io_context,
        std::move(on_read),
        std::move(on_write),
        std::move(on_connect),
        is_for_health_check
    );
    lb_client->connect(io_context, ip_address, port);
    return lb_client;
}

void LbClientManager::add_lb_client(LbClientPtr lb_client)
{
    std::unique_lock<std::shared_mutex> lock{m_lb_clients_mutex};
    m_lb_clients.insert({lb_client->get_id(), lb_client});
}

LbClientPtr LbClientManager::get_lb_client(const std::string &ip_port)
{
    std::shared_lock<std::shared_mutex> lock{m_lb_clients_mutex};
    auto it = m_lb_clients.find(ip_port);
    if (it == m_lb_clients.end())
    {
        return nullptr;
    }
    return it->second;
}

void LbClientManager::remove_lb_client(const std::string &ip_port)
{
    std::unique_lock<std::shared_mutex> lock{m_lb_clients_mutex};
    m_lb_clients.erase(ip_port);
}

} // namespace kload_balancer