#include "CompressionHeader.hpp"

namespace kcompress
{
std::ostream& operator<<(std::ostream &os, const CompressionHeader &header)
{
    os.write(reinterpret_cast<const char*>(&header.m_header_length), sizeof(header.m_header_length));
    os.write(reinterpret_cast<const char*>(&header.m_payload_length_bits), sizeof(header.m_payload_length_bits));
    os.write(reinterpret_cast<const char*>(&header.m_serialized_tree_total_bits), sizeof(header.m_serialized_tree_total_bits));
    os.write(reinterpret_cast<const char*>(&header.m_original_file_bytes), sizeof(header.m_original_file_bytes));
    os.write(reinterpret_cast<const char*>(header.m_serialized_tree.data()), header.m_serialized_tree.size());
    return os;
}

std::istream& operator>>(std::istream &is, CompressionHeader &header)
{
    is.read(reinterpret_cast<char*>(&header.m_header_length), sizeof(header.m_header_length));
    is.read(reinterpret_cast<char*>(&header.m_payload_length_bits), sizeof(header.m_payload_length_bits));
    is.read(reinterpret_cast<char*>(&header.m_serialized_tree_total_bits), sizeof(header.m_serialized_tree_total_bits));
    is.read(reinterpret_cast<char*>(&header.m_original_file_bytes), sizeof(header.m_original_file_bytes));

    auto serialized_tree_bytes_div_result = std::div(header.m_serialized_tree_total_bits, 8);
    if (serialized_tree_bytes_div_result.rem != 0)
    {
        serialized_tree_bytes_div_result.quot++;
    }
    header.m_serialized_tree.resize(serialized_tree_bytes_div_result.quot);
    is.read(reinterpret_cast<char*>(header.m_serialized_tree.data()), serialized_tree_bytes_div_result.quot);
    return is;
}
} // namespace kcompress