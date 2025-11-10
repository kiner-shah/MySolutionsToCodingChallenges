#include "TcpBuffer.hpp"

namespace kredis
{
TcpBuffer::TcpBuffer(std::size_t capacity)
    : m_buffer(capacity, '\0'),
    m_start_pos{0},
    m_end_pos{0}
{
}

std::errc TcpBuffer::append(const char *data, std::size_t length)
{
    if (m_end_pos + length > m_buffer.size())
    {
        return std::errc::message_size;
    }
    std::copy(data, data + length, m_buffer.begin() + m_end_pos);
    m_end_pos += length;
    return std::errc{};
}

std::string_view TcpBuffer::view() const
{
    return std::string_view{m_buffer.begin() + m_start_pos, m_buffer.begin() + m_end_pos};
}

void TcpBuffer::consume(std::size_t length)
{
    m_start_pos += length;
    if (m_start_pos == m_end_pos)
    {
        m_start_pos = 0;
        m_end_pos = 0;
    }
    else if (m_start_pos > m_buffer.size() / 2)
    {
        std::move(m_buffer.begin() + m_start_pos, m_buffer.begin() + m_end_pos, m_buffer.begin());
        m_end_pos -= m_start_pos;
        m_start_pos = 0;
    }
}

std::size_t TcpBuffer::size() const
{
    return m_end_pos - m_start_pos;
}

bool TcpBuffer::empty() const
{
    return size() == 0;
}

} // namespace kredis