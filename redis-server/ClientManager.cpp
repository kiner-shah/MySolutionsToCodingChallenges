#include "ClientManager.hpp"

namespace kredis
{
ClientManager::ClientManager(std::shared_ptr<spdlog::logger> logger)
    : m_work_guard{asio::make_work_guard(m_io_context)},
    m_threads(std::thread::hardware_concurrency()),
    m_logger{std::move(logger)},
    m_client_counter{0}
{
    for (auto& thread : m_threads)
    {
        thread = std::thread([this]()
        {
            m_io_context.run();
        });
    }
}

ClientManager::~ClientManager()
{
    m_work_guard.reset();
    while (!m_io_context.stopped())
    {
        m_io_context.stop();
    }

    for (auto& thread : m_threads)
    {
        thread.join();
    }
}

ClientManager::ClientPtr ClientManager::create_new_client(
    ReadCallbackType on_read,
    WriteCallbackType on_write)
{
    std::unique_lock<std::shared_mutex> lock{m_clients_mutex};
    std::string new_user_client_id = "Client_" + std::to_string(m_client_counter);
    m_client_counter++;
    auto client = std::make_shared<Client>(
        new_user_client_id,
        m_io_context,
        m_logger,
        std::move(on_read),
        std::move(on_write)
    );
    auto result = m_clients.insert({new_user_client_id, std::move(client)});
    return result.first->second;
}

ClientManager::ClientPtr ClientManager::get_client(const std::string& id)
{
    std::shared_lock<std::shared_mutex> lock{m_clients_mutex};
    auto it = m_clients.find(id);
    if (it != m_clients.end())
    {
        return it->second;
    }
    return nullptr;
}

void ClientManager::remove_client(const std::string &id)
{
    std::unique_lock<std::shared_mutex> lock{m_clients_mutex};
    m_clients.erase(id);
}

}   // namespace kredis