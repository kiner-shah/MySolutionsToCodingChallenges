#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <vector>
#include <bitset>
#include "HuffmanTree.hpp"
#include "CompressionHeader.hpp"
#include "utils.hpp"

namespace
{
struct Config
{
    std::string input_file_path = {};
    std::string output_file_path = {};
    bool should_decode = false;

    friend std::ostream& operator<<(std::ostream& os, const Config& config);
};

std::ostream& operator<<(std::ostream& os, const Config& config)
{
    os << "Input file path: " << config.input_file_path << "\nOutput file path: " << config.output_file_path
        << "\nShould decode?: " << std::boolalpha << config.should_decode;
    return os;
}

void compute_frequency(std::string_view file_contents, std::unordered_map<unsigned char, std::uint64_t>& frequency_map)
{
    for (unsigned char c : file_contents)
    {
        frequency_map[c]++;
    }
}

void print_usage(std::string program_name)
{
    std::cout << "Usage: " << program_name << " -d|-e filename -o outputfilename\n";
    std::cout << "\n  -d|-e filename\tDecode/encode the given file"
                "\n  -o outputfilename\tName of the file where output needs to be stored\n";
}

// void print_bitmap(const std::unordered_map<unsigned char, std::string>& bitmap)
// {
//     for (const auto& [c, value] : bitmap)
//     {
//         std::cout << std::bitset<8>(c) << ' ' << value << '\n';
//     }
// }
}   // namespace

int main(int argc, char** argv)
{
    if (argc != 5)
    {
        print_usage(argv[0]);
        return 1;
    }

    Config config;
    for (int i = 1; i < argc;)
    {
        std::string argument{argv[i]};
        if (argument == "-d" || argument == "-e")
        {
            if (argument[1] == 'd')
            {
                config.should_decode = true;
            }
            config.input_file_path = argv[++i];
            ++i;
        }
        else if (argument == "-o")
        {
            config.output_file_path = argv[++i];
            ++i;
        }
    }
    std::cout << config << '\n';

    // Read a file
    std::size_t file_size = std::filesystem::file_size(config.input_file_path);
    std::string buffer(file_size, '\0');
    std::ifstream input_file(config.input_file_path, std::ios::binary);
    if (!input_file.good())
    {
        std::cerr << "Cannot open input file " << config.input_file_path << '\n';
        return 1;
    }
    if (!input_file.read(buffer.data(), file_size))
    {
        std::cerr << "Failure during reading input file " << config.input_file_path << '\n';
        input_file.close();
        return 1;
    }
    input_file.close();

    std::ofstream output_file(config.output_file_path, std::ios::binary);
    if (!output_file.good())
    {
        std::cerr << "Cannot open output file " << config.output_file_path << '\n';
        return 1;
    }

    if (!config.should_decode)
    {
        // Encoding

        // Compute frequency
        std::unordered_map<unsigned char, std::uint64_t> frequency_map;
        compute_frequency(buffer, frequency_map);

        // Build tree
        kcompress::HuffmanTree huffman_tree{frequency_map};
        frequency_map.clear();
        // huffman_tree.print_tree();

        // Serialize data
        auto bit_map = huffman_tree.get_bit_map();
        // print_bitmap(bit_map);
        std::uint64_t total_bits = 0;
        auto output_buffer = huffman_tree.serialize_payload(std::vector<unsigned char>(buffer.begin(), buffer.end()), total_bits);

        // Serialize tree
        kcompress::CompressionHeader header;
        header.m_original_file_bytes = file_size;
        header.m_serialized_tree = huffman_tree.serialize(header.m_serialized_tree_total_bits);
        header.m_payload_length_bits = total_bits;
        header.m_header_length = 24u + header.m_serialized_tree.size();
        // std::cout << header.m_serialized_tree.size() << ' ' << header.m_serialized_tree_total_bits << '\n';
        // std::cout << "Header length: " << std::hex << header.m_header_length << std::dec << '\n';
        // std::cout << "Original file bytes: " << std::hex << header.m_original_file_bytes << std::dec << '\n';
        // std::cout << "Total bits in serialized tree: " << std::hex << header.m_serialized_tree_total_bits << std::dec << '\n';
        // std::cout << "Serialized tree bytes: " << header.m_serialized_tree.size() << '\n';
        // std::cout << "Payload bytes: " << output_buffer.size() << '\n';
        // std::cout << "Payload bits: " << std::hex << header.m_payload_length_bits << std::dec << '\n';
        // for (unsigned char c : header.m_serialized_tree)
        // {
        //     std::cout << std::bitset<8>(c) << ' ';
        // } std::cout << '\n';

        output_file << header;
        for (auto output_buffer_byte : output_buffer)
        {
            // std::cout << std::bitset<8>(output_buffer_byte) << ' ';
            output_file << output_buffer_byte;
        }
        output_file.close();
    }
    else
    {
        // Decoding
        std::istringstream ss{buffer};

        kcompress::CompressionHeader header;
        if (!(ss >> header))
        {
            std::cerr << "Failure while reading compressed file header\n";
            return 1;
        }

        auto serialized_payload_bytes_div_result = std::div(header.m_payload_length_bits, 8);
        if (serialized_payload_bytes_div_result.rem != 0)
        {
            serialized_payload_bytes_div_result.quot++;
        }

        std::vector<unsigned char> payload_buffer;
        payload_buffer.resize(serialized_payload_bytes_div_result.quot);
        if (!ss.read(reinterpret_cast<char*>(payload_buffer.data()), serialized_payload_bytes_div_result.quot))
        {
            std::cerr << "Failure while reading compressed file payload\n";
            return 1;
        }

        // std::cout << header.m_serialized_tree.size() << ' ' << header.m_serialized_tree_total_bits << '\n';

        kcompress::HuffmanTree huffman_tree;
        huffman_tree.deserialize(header.m_serialized_tree, header.m_serialized_tree_total_bits);

        auto bit_map = huffman_tree.get_bit_map();
        // print_bitmap(bit_map);

        auto bytes = huffman_tree.deserialize_payload(payload_buffer, header.m_payload_length_bits);
        for (auto byte : bytes)
        {
            output_file << byte;
        }
        // auto codepoints = huffman_tree.deserialize_payload(payload_buffer, header.m_payload_length_bits);
        // for (const auto& codepoint : codepoints)
        // {
        //     std::array<unsigned char, 4> bytes;
        //     auto converted_bytes_length = kcompress::convert_utf32_to_utf8_char(codepoint, bytes);
        //     for (unsigned int bytes_index = 0; bytes_index < converted_bytes_length; bytes_index++)
        //     {
        //         output_file << bytes[bytes_index];
        //     }
        // }
        output_file.close();
    }
}
