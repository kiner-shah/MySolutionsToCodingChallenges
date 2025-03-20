#pragma once

#include <cstdint>
#include <ostream>
#include <vector>

namespace kcompress
{
struct CompressionHeader
{
    std::uint64_t m_header_length = 0;
    std::uint64_t m_payload_length_bits = 0;
    std::uint64_t m_serialized_tree_total_bits = 0;
    std::uint64_t m_original_file_bytes = 0;
    std::vector<unsigned char> m_serialized_tree;

    friend std::ostream& operator<<(std::ostream& os, const CompressionHeader& header);
};
}   // namespace kcompress