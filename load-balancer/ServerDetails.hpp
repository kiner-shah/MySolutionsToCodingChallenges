#pragma once

#include <string_view>
#include <string>

namespace kload_balancer
{
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
    std::string get_endpoint() const;
    void set_available(bool status);
};
}   // namespace kload_balancer