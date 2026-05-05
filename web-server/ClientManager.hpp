#pragma once

#include "Client.hpp"
#include <shared_mutex>
#include <unordered_map>

namespace kweb_server
{
class ClientManager
{
    std::unordered_map<std::string, ClientPtr> m_clients;
    std::uint64_t m_client_counter = 0;
    std::shared_mutex m_mutex;
public:
    ClientPtr create_new_client(asio::io_context& io_context, ClientCallbackType on_read, ClientCallbackType on_write);
    ClientPtr get_client(const std::string& client_id);
    void remove_client(const std::string& client_id);
};
}   // namespace kweb_server