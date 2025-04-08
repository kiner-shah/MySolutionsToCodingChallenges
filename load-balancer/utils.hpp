#pragma once

#include <array>
#include <string_view>

namespace kload_balancer
{
constexpr std::array<unsigned char, 2048> get_buffer_from_message(std::string_view message)
{
    std::array<unsigned char, 2048> buffer{};
    for (std::size_t index = 0; index < message.size(); index++)
    {
        buffer[index] = message[index];
    }
    return buffer;
}

constexpr std::array<unsigned char, 2048> get_service_unavailable_message()
{
    constexpr std::string_view message{"HTTP/1.1 503 Service Unavailable\r\nContent-Type: text/html;\r\nContent-Length: 23\r\n\r\n503 Service Unavailable"};
    return get_buffer_from_message(message);
}

constexpr std::array<unsigned char, 2048> get_internal_server_error_message()
{
    constexpr std::string_view message{"HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/html;\r\nContent-Length: 25\r\n\r\n500 Internal Server Error"};
    return get_buffer_from_message(message);
}
}   // namespace kload_balancer