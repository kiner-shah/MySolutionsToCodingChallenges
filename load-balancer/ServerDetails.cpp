#include "ServerDetails.hpp"

namespace kload_balancer
{
ServerDetails::ServerDetails(std::string_view ip, std::string_view port)
    : m_ip{std::move(ip)}, m_port{std::move(port)}, m_is_available{false}
{
}

bool ServerDetails::is_available() const
{
    return m_is_available;
}

std::string_view ServerDetails::get_ip() const
{
    return m_ip;
}

std::string_view ServerDetails::get_port() const
{
    return m_port;
}

std::string ServerDetails::get_endpoint() const
{
    return std::string{m_ip} + ':' + std::string{m_port};
}

void ServerDetails::set_available(bool status)
{
    m_is_available = status;
}
}   // namespace kload_balancer