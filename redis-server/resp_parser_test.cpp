#include "RespParser.hpp"
#include <iostream>
#include <fstream>
#include <spdlog/sinks/stdout_color_sinks.h>

std::string unescape_string(const std::string& str) {
    std::string result = str;
    size_t pos = 0;
    while ((pos = result.find("\\r", pos)) != std::string::npos)
    {
        result.replace(pos, 2, "\r");
        pos++;
    }
    pos = 0;
    while ((pos = result.find("\\n", pos)) != std::string::npos)
    {
        result.replace(pos, 2, "\n");
        pos++;
    }
    return result;
}

int main()
{
    auto logger = spdlog::stdout_color_mt("RedisServer");
    logger->set_level(spdlog::level::debug);

    kredis::RespParser parser{logger};
    std::ifstream input_file("../valid_tests_resp.txt");
    std::vector<std::string> lines;
    if (input_file)
    {
        std::string line;
        while (std::getline(input_file, line))
        {
            lines.push_back(line);
        }
        input_file.close();
    }
    for (const auto& line : lines)
    {
        std::string unescaped_line = unescape_string(line);
        std::cout << "Parsing line " << line << '\n';
        kredis::RespType result;
        if (!parser.parse(unescaped_line, result))
        {
            std::cerr << "Parse failed\n";
            continue;
        }
        if (std::holds_alternative<kredis::RespArray>(result))
        {
            const auto& array = std::get<kredis::RespArray>(result);
            bool is_first = false;
            std::cout << '[';
            for (const auto& element : array)
            {
                if (is_first)
                {
                    std::cout << ", ";
                }
                std::visit([](const auto& value) { std::cout << value; }, element);
                if (!is_first)
                {
                    is_first = true;
                }
            }
            std::cout << ']' << '\n';
        }
        else if (std::holds_alternative<kredis::RespError>(result))
        {
            const auto& error = std::get<kredis::RespError>(result);
            std::cout << "Error: " << error << '\n';
        }
        else if (std::holds_alternative<kredis::RespInt>(result))
        {
            const auto& integer = std::get<kredis::RespInt>(result);
            std::cout << "Integer: " << integer << '\n';
        }
        else if (std::holds_alternative<kredis::RespString>(result))
        {
            const auto& string = std::get<kredis::RespString>(result);
            std::cout << string << '\n';
        }
        else if (std::holds_alternative<kredis::RespNull>(result))
        {
            [[maybe_unused]] const auto& null = std::get<kredis::RespNull>(result);
            std::cout << "NULL\n";
        }
    }
    return 0;
}