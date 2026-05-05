#include "ClientManager.hpp"
#include <mutex>

namespace kweb_server
{
ClientPtr ClientManager::create_new_client(asio::io_context &io_context, ClientCallbackType on_read, ClientCallbackType on_write)
{
    std::unique_lock<std::shared_mutex> lock{m_mutex};
    std::string client_id = "client_" + std::to_string(m_client_counter++);
    auto result = m_clients.insert({
        client_id,
        std::make_shared<Client>(client_id, io_context, std::move(on_read), std::move(on_write))
    });
    return result.first->second;
}

ClientPtr ClientManager::get_client(const std::string &client_id)
{
    std::shared_lock lock{m_mutex};
    auto it = m_clients.find(client_id);
    if (it == m_clients.end())
    {
        return nullptr;
    }
    return it->second;
}

void ClientManager::remove_client(const std::string &client_id)
{
    std::unique_lock lock{m_mutex};
    m_clients.erase(client_id);
}
}   // namespace kweb_server