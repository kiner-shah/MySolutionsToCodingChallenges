#pragma once

#include <cstdint>
#include <ostream>

namespace kcompress
{
struct CompressionHeader
{
    std::uint64_t m_header_length;
    std::uint64_t m_payload_length;
    std::uint64_t m_original_file_bytes;

    friend std::ostream& operator<<(std::ostream& os, const CompressionHeader& header);
};
}   // namespace kcompress