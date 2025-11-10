#pragma once

#include <string>
#include <system_error>

namespace kredis
{
class TcpBuffer
{
    std::string m_buffer;
    std::size_t m_start_pos;
    std::size_t m_end_pos;
public:
    explicit TcpBuffer(std::size_t capacity = 8192);
    std::errc append(const char* data, std::size_t length);
    std::string_view view() const;
    void consume(std::size_t length);
    std::size_t size() const;
    bool empty() const;
};
}   // namespace kredis