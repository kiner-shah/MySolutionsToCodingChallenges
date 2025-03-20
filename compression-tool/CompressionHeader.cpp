#include "CompressionHeader.hpp"

namespace kcompress
{
std::ostream& operator<<(std::ostream &os, const CompressionHeader &header)
{
    os.write(reinterpret_cast<const char*>(&header.m_header_length), sizeof(header.m_header_length));
    os.write(reinterpret_cast<const char*>(&header.m_payload_length_bits), sizeof(header.m_payload_length_bits));
    os.write(reinterpret_cast<const char*>(&header.m_serialized_tree_total_bits), sizeof(header.m_serialized_tree_total_bits));
    os.write(reinterpret_cast<const char*>(&header.m_original_file_bytes), sizeof(header.m_original_file_bytes));
    for (auto c : header.m_serialized_tree)
    {
        os << c;
    }
    return os;
}
}   // namespace kcompress