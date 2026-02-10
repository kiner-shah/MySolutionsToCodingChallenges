#pragma once

#include "UserClient.hpp"
#include <asio/io_context.hpp>
#include <shared_mutex>
#include <unordered_map>

namespace kload_balancer
{
class UserClientManager
{
    std::unordered_map<std::string, UserClientPtr> m_user_clients;
    std::uint64_t m_user_client_counter;
    std::shared_mutex m_user_clients_mutex;
public:
    UserClientManager();
    ~UserClientManager() = default;
    UserClientPtr create_new_user_client(asio::io_context& io_context, UserClientCallbackType on_read, UserClientCallbackType on_write);
    UserClientPtr get_user_client(const std::string& id);
    void remove_user_client(const std::string& id);
};
}   // namespace kload_balancer