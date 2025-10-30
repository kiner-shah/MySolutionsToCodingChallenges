#include "RespParser.hpp"
#include <iostream>
#include <fstream>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace
{
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

void print_resp_array(const kredis::RespArray& array);

struct RespTypeVisitor
{
    void operator()(const kredis::RespArray& array) const
    {
        print_resp_array(array);
    }
    void operator()(const kredis::RespError& error) const
    {
        std::cout << "Error: " << error;
    }
    void operator()(const kredis::RespInt& integer) const
    {
        std::cout << "Integer: " << integer;
    }
    void operator()(const kredis::RespString& str) const
    {
        std::cout << "String: " << str;
    }
    void operator()(const kredis::RespNull& null) const
    {
        std::cout << "NULL";
    }
};

void print_resp_array(const kredis::RespArray& array)
{
    RespTypeVisitor visitor;
    bool is_first = false;
    std::cout << '[';
    for (const auto& element : array)
    {
        if (is_first)
        {
            std::cout << ", ";
        }
        std::visit(visitor, element);
        if (!is_first)
        {
            is_first = true;
        }
    }
    std::cout << ']';
}
}   // namespace

int main()
{
    auto logger = spdlog::stdout_color_mt("RedisServer");
    logger->set_level(spdlog::level::debug);

    kredis::RespParser parser{logger};
    std::ifstream input_file("../invalid_tests_resp.txt");
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
        std::vector<kredis::RespParserResult> results;
        if (!parser.parse(unescaped_line, results))
        {
            std::cerr << "Parse failed: ";
            for (const auto& result : results)
            {
                std::cerr << kredis::resp_parser_state_to_string(result.m_state) << ' ';
            }
            std::cerr << '\n';
            continue;
        }
        std::size_t index = 0;
        for (const auto& result : results)
        {
            std::cout << "Index: " << index << ' ';
            index++;
            if (!result.m_type.has_value())
            {
                std::cerr << "Parse failed: " << resp_parser_state_to_string(result.m_state) << '\n';
                continue;
            }
            auto type = *result.m_type;
            RespTypeVisitor visitor;

            std::visit(visitor, type);
            std::cout << '\n';
        }
    }
    return 0;
}