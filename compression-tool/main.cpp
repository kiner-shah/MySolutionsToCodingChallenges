#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <vector>
#include <bitset>
#include "HuffmanTree.hpp"
#include "CompressionHeader.hpp"

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

void compute_frequency(const std::vector<unsigned char>& file_contents, std::array<std::uint64_t, 256>& frequency_map)
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
    std::ifstream input_file(config.input_file_path, std::ios::binary);
    if (!input_file.good())
    {
        std::cerr << "Cannot open input file " << config.input_file_path << '\n';
        return 1;
    }

    std::ofstream output_file(config.output_file_path, std::ios::binary);
    if (!output_file.good())
    {
        std::cerr << "Cannot open output file " << config.output_file_path << '\n';
        return 1;
    }

    if (!config.should_decode)
    {
        // Encoding

        std::vector<unsigned char> buffer(file_size, '\0');
        if (!input_file.read(reinterpret_cast<char*>(buffer.data()), file_size))
        {
            std::cerr << "Failure during reading input file " << config.input_file_path << '\n';
            input_file.close();
            return 1;
        }
        input_file.close();

        // Compute frequency
        std::array<std::uint64_t, 256> frequency_map{};
        compute_frequency(buffer, frequency_map);

        // Build tree
        kcompress::HuffmanTree huffman_tree{frequency_map};
        // huffman_tree.print_tree();

        // Serialize data
        std::uint64_t total_bits = 0;
        auto output_buffer = huffman_tree.serialize_payload(buffer, total_bits);

        // Serialize tree
        kcompress::CompressionHeader header;
        header.m_original_file_bytes = file_size;
        header.m_serialized_tree = huffman_tree.serialize(header.m_serialized_tree_total_bits);
        header.m_payload_length_bits = total_bits;
        header.m_header_length = 24u + header.m_serialized_tree.size();

        output_file << header;
        output_file.write(reinterpret_cast<const char*>(output_buffer.data()), output_buffer.size());
        output_file.close();
    }
    else
    {
        // Decoding

        kcompress::CompressionHeader header;
        if (!(input_file >> header))
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
        if (!input_file.read(reinterpret_cast<char*>(payload_buffer.data()), serialized_payload_bytes_div_result.quot))
        {
            std::cerr << "Failure while reading compressed file payload\n";
            return 1;
        }
        input_file.close();

        kcompress::HuffmanTree huffman_tree;
        huffman_tree.deserialize(header.m_serialized_tree, header.m_serialized_tree_total_bits);

        auto bytes = huffman_tree.deserialize_payload(payload_buffer, header.m_payload_length_bits, header.m_original_file_bytes);
        output_file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
        output_file.close();
    }
}
