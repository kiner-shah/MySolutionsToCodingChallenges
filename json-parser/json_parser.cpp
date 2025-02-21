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
    std::cout << "Tokens size: " << (token_sequence.size() * sizeof(kjson::Token)) << '\n';
    // kjson::Parser parser;
    // return parser.parse(token_sequence);
    // for (const auto& token : token_sequence)
    // {
    //     std::cout << token << '\n';
    // }
    return true;
}
}   // namespace

int main(int argc, char** argv)
{
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
            return 1;
        }
        std::ifstream json_file(json_file_path, std::ios::binary);
        if (!json_file)
        {
            std::cerr << "File coundn't be opened: " << json_file_path << '\n';
            return 1;
        }
        result = parse_json(json_file);
        json_file.close();
    }
    if (!result)
    {
        return 1;
    }
    return 0;
}