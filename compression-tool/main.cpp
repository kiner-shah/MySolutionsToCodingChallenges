#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <bitset>
#include "HuffmanTree.hpp"
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
    // Reference: https://writings.sh/post/en/utf8

    char32_t utf32_codepoint;
    unsigned int state = 0;
    std::vector<char32_t> codepoints;

    for (unsigned char c : file_contents)
    {
        // Decoding UTF-8 bytes into UTF-32 codepoints
        switch (state)
        {
            case 0:
                if (c >= 0x0 && c <= 0x7f)
                {
                    utf32_codepoint = c;
                }
                else if (c >= 0xc2 && c <= 0xdf)
                {
                    state = 1;
                    utf32_codepoint = c & 0x1f;
                }
                else if (c == 0xe0)
                {
                    state = 4;
                    utf32_codepoint = c & 0xf;
                }
                else if (c >= 0xe1 && c <= 0xef)
                {
                    state = 2;
                    utf32_codepoint = c & 0xf;
                }
                else if (c == 0xf0)
                {
                    state = 5;
                    utf32_codepoint = c & 0x7;
                }
                else if (c >= 0xf1 && c <= 0xf3)
                {
                    state = 3;
                    utf32_codepoint = c & 0x7;
                }
                else if (c == 0xf4)
                {
                    state = 6;
                    utf32_codepoint = c & 0x7;
                }
                else
                {
                    state = 8;
                }
                break;
            case 1:
            case 2:
            case 3:
                if (c >= 0x80 && c <= 0xbf)
                {
                    state--;
                    utf32_codepoint = (utf32_codepoint << 6) | (c & 0x3f);
                }
                else
                {
                    state = 8;
                }
                break;
            case 4:
                if (c >= 0xa0 && c <= 0xbf)
                {
                    state = 1;
                    utf32_codepoint = (utf32_codepoint << 6) | (c & 0x3f);
                }
                else
                {
                    state = 8;
                }
                break;
            case 5:
                if (c >= 0x90 && c <= 0xbf)
                {
                    state = 2;
                    utf32_codepoint = (utf32_codepoint << 6) | (c & 0x3f);
                }
                else
                {
                    state = 8;
                }
                break;
            case 6:
                if (c >= 0x80 && c <= 0x8f)
                {
                    state = 2;
                    utf32_codepoint = (utf32_codepoint << 6) | (c & 0x3f);
                }
                else
                {
                    state = 8;
                }
                break;
            default:
                state = 8;
                break;
        }

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
    std::ifstream input_file(config.input_file_path);
    if (!input_file.good())
    {
        std::cerr << "Cannot open input file " << config.input_file_path << '\n';
        return 1;
    }
    if (!input_file.read(buffer.data(), file_size))
    {
        std::cerr << "Failure during reading input file " << config.input_file_path << '\n';
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
        auto bit_map = huffman_tree.get_bit_map();

    }
    else
    {
        // Decoding

    }
}
