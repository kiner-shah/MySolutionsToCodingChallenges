#include <iostream>
#include <fstream>
#include <filesystem>
#include "Parser.hpp"

namespace
{
bool validate_file(const std::string& json_file_path)
{
    if (!std::filesystem::exists(json_file_path))
    {
        std::cerr << "File not found: " << json_file_path << '\n';
        return false;
    }
    if (!std::filesystem::is_regular_file(json_file_path))
    {
        std::cerr << "File isn't a regular file: " << json_file_path << '\n';
        return false;
    }
    if (std::filesystem::is_empty(json_file_path))
    {
        std::cerr << "File is empty: " << json_file_path << '\n';
        return false;
    }
    return true;
}

bool parse_json(std::istream& input)
{
    kjson::Tokenizer tokenizer;
    auto token_sequence = tokenizer.tokenize(input);
    if (token_sequence.empty())
    {
        return false;
    }
    // Uncomment for debugging
    // for (size_t i = 0; i < token_sequence.size(); i++)
    // {
    //     std::cout << i << ' ' << token_sequence[i] << '\n';
    // }
    kjson::Parser parser;
    return parser.parse(token_sequence);
}
}   // namespace

int main(int argc, char** argv)
{
    if (argc < 1 || argc > 2)
    {
        std::cerr << "Usage: " << argv[0] << " [filename]\n";
        std::cerr << "\nfilename\tInput file path. This is optional, in absence of this path, input will be taken from stdin\n";
        return 1;
    }

    bool result = false;
    if (argc == 1)
    {
        result = parse_json(std::cin);
    }
    else if (argc == 2)
    {
        std::string json_file_path{argv[1]};
        if (!validate_file(json_file_path))
        {
            std::cout << "FAILED\n";
            return 1;
        }
        std::ifstream json_file(json_file_path, std::ios::binary);
        if (!json_file)
        {
            std::cerr << "File coundn't be opened: " << json_file_path << '\n';
            std::cout << "FAILED\n";
            return 1;
        }
        result = parse_json(json_file);
        json_file.close();
    }
    if (!result)
    {
        std::cout << "FAILED\n";
        return 1;
    }
    std::cout << "PASSED\n";
    return 0;
}