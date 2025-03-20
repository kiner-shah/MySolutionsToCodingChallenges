#include <iostream>
#include <fstream>
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
    std::string input_file_path;
    std::string output_file_path;
    bool should_decode = false;

    friend std::ostream& operator<<(std::ostream& os, const Config& config);
};

std::ostream& operator<<(std::ostream& os, const Config& config)
{
    os << "Input file path: " << config.input_file_path << "\nOutput file path: " << config.output_file_path
        << "\nShould decode?: " << std::boolalpha << config.should_decode;
    return os;
}

// Fills the map with counts of each UTF-32 codepoint and returns the converted sequence of UTF-32 codepoints.
std::vector<char32_t> process_file_and_compute_frequency(const std::string& file_contents, std::unordered_map<char32_t, std::uint64_t>& char32_frequency_map)
{
    char32_t utf32_codepoint;
    unsigned int state = 0;
    std::vector<char32_t> codepoints;

    for (unsigned char c : file_contents)
    {
        state = kcompress::decode_utf8_char_to_utf32(c, state, utf32_codepoint);

        if (state == 8)
        {
            std::cerr << "Failure: bad encoding\n";
            return std::vector<char32_t>{};
        }
        else if (state == 0)
        {
            codepoints.push_back(utf32_codepoint);
            char32_frequency_map[utf32_codepoint]++;
        }
    }
    
    return codepoints;
}

void print_usage(std::string program_name)
{
    std::cout << "Usage: " << program_name << " -d|-e filename -o outputfilename\n";
    std::cout << "\n  -d|-e filename\tDecode/encode the given file"
                "\n  -o outputfilename\tName of the file where output needs to be stored\n";
}
}   // namespace

int main(int argc, char** argv)
{
    if (argc != 5)
    {
        print_usage(argv[0]);
        return 1;
    }
 
    std::cout << "Setting locale to " << std::setlocale(LC_ALL, "") << '\n';
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

    std::ofstream output(config.output_file_path, std::ios::binary);
    if (!output.good())
    {
        std::cerr << "Cannot open output file " << config.output_file_path << '\n';
        return 1;
    }

    if (!config.should_decode)
    {
        // Encoding

        // Convert multi-byte characters to wide char - store count of each char in a map
        // Create individual tree nodes

        std::unordered_map<char32_t, std::uint64_t> char32_frequency_map;
        auto codepoint_sequence = process_file_and_compute_frequency(buffer, char32_frequency_map);
        if (codepoint_sequence.empty())
        {
            return 1;
        }
        //std::cout << converted_input.size() << '\n';

        // Build tree
        kcompress::HuffmanTree huffman_tree{char32_frequency_map};
        char32_frequency_map.clear();
        // huffman_tree.print_tree();

        // Serialize data
        auto bit_map = huffman_tree.get_bit_map();
        unsigned char byte;
        unsigned char remaining_bits = 8;
        std::uint64_t total_bits = 0;
        std::vector<unsigned char> output_buffer;
        for (auto codepoint : codepoint_sequence)
        {
            std::string code = bit_map[codepoint];
            for (unsigned char c : code)
            {
                if (c == '0')
                {
                    byte <<= 1;
                }
                else if (c == '1')
                {
                    byte = (byte << 1) | 1;
                }
                total_bits++;
                remaining_bits--;
                if (remaining_bits == 0)
                {
                    remaining_bits = 8;
                    output_buffer.push_back(byte);
                    byte = 0;
                }
            }
        }
        if (remaining_bits != 0 && remaining_bits != 8)
        {
            output_buffer.push_back(byte << remaining_bits);
        }

        // Serialize tree
        kcompress::CompressionHeader header;
        header.m_original_file_bytes = file_size;
        header.m_serialized_tree = huffman_tree.serialize(header.m_serialized_tree_total_bits);
        header.m_payload_length_bits = total_bits;
        header.m_header_length = 24u + header.m_serialized_tree.size();
        // std::cout << "Header length: " << std::hex << header.m_header_length << std::dec << '\n';
        // std::cout << "Original file bytes: " << std::hex << header.m_original_file_bytes << std::dec << '\n';
        // std::cout << "Total bits in serialized tree: " << std::hex << header.m_serialized_tree_total_bits << std::dec << '\n';
        // std::cout << "Payload bytes: " << output_buffer.size() << '\n';
        // std::cout << "Payload bits: " << std::hex << header.m_payload_length_bits << std::dec << '\n';
        // for (unsigned char c : header.m_serialized_tree)
        // {
        //     std::bitset<8> b(c);
        //     std::cout << b << ' ';
        // }

        output << header;
        for (auto output_buffer_byte : output_buffer)
        {
            output << output_buffer_byte;
        }
        output.close();
    }
    else
    {
        // Decoding

        // Read header
        //  If 1, create a tree leaf. Then read a byte, check if state is 0
        //    If state is 0, then set converted codepoint to tree leaf
        //    Else continue reading bytes
        //  Else if 0, create a non-leaf. For left and right, recursively continue for both.
    }
}
