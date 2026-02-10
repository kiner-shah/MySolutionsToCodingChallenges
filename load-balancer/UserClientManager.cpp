#include "UserClientManager.hpp"
#include <mutex>

namespace kload_balancer
{
UserClientManager::UserClientManager()
    : m_user_client_counter{0}
{
}

UserClientPtr UserClientManager::create_new_user_client(asio::io_context &io_context, UserClientCallbackType on_read, UserClientCallbackType on_write)
{
    std::unique_lock<std::shared_mutex> lock{m_user_clients_mutex};
    std::string id = "UserClient_" + std::to_string(m_user_client_counter++);
    auto user_client = std::make_shared<UserClient>(
        id,
        io_context,
        std::move(on_read),
        std::move(on_write)
    );
    auto result = m_user_clients.insert({id, user_client});
    return result.first->second;
}

UserClientPtr UserClientManager::get_user_client(const std::string &id)
{
    std::shared_lock<std::shared_mutex> lock{m_user_clients_mutex};
    auto it = m_user_clients.find(id);
    if (it == m_user_clients.end())
    {
        return nullptr;
    }
    return it->second;
}

void UserClientManager::remove_user_client(const std::string &id)
{
    std::unique_lock<std::shared_mutex> lock{m_user_clients_mutex};
    m_user_clients.erase(id);
}
}   // namespace kload_balancer