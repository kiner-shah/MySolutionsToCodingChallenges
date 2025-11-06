#include "RespGenerator.hpp"
#include <spdlog/fmt/fmt.h>

namespace kredis
{
std::size_t RespGenerator::calculate_resp_string_size(const RespString &input) const
{
    if (input.m_is_bulk)
    {
        // Bulk string: $<length>\r\n<value>\r\n
        std::size_t length_string_size = std::to_string(input.m_str.size()).size();
        return 5 + input.m_str.size() + length_string_size;
    }
    // Simple string: +<value>\r\n
    return 3 + input.m_str.size();
}

std::size_t RespGenerator::calculate_resp_array_size(const RespArray &input) const
{
    // *<array len>\r\n
    std::size_t array_len_string_size = std::to_string(input.size()).size();
    std::size_t array_string_size = 3 + array_len_string_size;

    for (const auto& element : input)
    {
        if (std::holds_alternative<RespString>(element))
        {
            array_string_size += calculate_resp_string_size(std::get<RespString>(element));
        }
        else if (std::holds_alternative<RespInt>(element))
        {
            array_string_size += calculate_resp_int_size(std::get<RespInt>(element));
        }
        else if (std::holds_alternative<RespNull>(element))
        {
            array_string_size += calculate_resp_null_size(std::get<RespNull>(element));
        }
        else if (std::holds_alternative<RespError>(element))
        {
            array_string_size += calculate_resp_error_size(std::get<RespError>(element));
        }
        else if (std::holds_alternative<RespArray>(element))
        {
            array_string_size += calculate_resp_array_size(std::get<RespArray>(element));
        }
    }
    return array_string_size;
}

std::size_t RespGenerator::calculate_resp_error_size(const RespError &input) const
{
    // Error: -<value>\r\n
    return 3 + input.size();
}

std::size_t RespGenerator::calculate_resp_int_size(const RespInt &input) const
{
    // Int: :<number>\r\n
    std::size_t int_string_size = std::to_string(input).size();
    return 3 + int_string_size;
}

std::size_t RespGenerator::calculate_resp_null_size(const RespNull &input) const
{
    // Null: _\r\n
    return 3;
}

std::string RespGenerator::generate_resp_string(const RespString &input) const
{
    if (input.m_is_bulk)
    {
        return fmt::format("${}\r\n{}\r\n", input.m_str.size(), input.m_str);
    }
    return fmt::format("+{}\r\n", input.m_str);
}

std::string RespGenerator::generate_resp_array(const RespArray &input) const
{
    std::string result;
    result.reserve(calculate_resp_array_size(input));

    result = fmt::format("*{}\r\n", input.size());
    for (const auto& element : input)
    {
        if (std::holds_alternative<RespString>(element))
        {
            result += generate_resp_string(std::get<RespString>(element));
        }
        else if (std::holds_alternative<RespInt>(element))
        {
            result += generate_resp_int(std::get<RespInt>(element));
        }
        else if (std::holds_alternative<RespNull>(element))
        {
            result += generate_resp_null(std::get<RespNull>(element));
        }
        else if (std::holds_alternative<RespError>(element))
        {
            result += generate_resp_error(std::get<RespError>(element));
        }
        else if (std::holds_alternative<RespArray>(element))
        {
            result += generate_resp_array(std::get<RespArray>(element));
        }
    }
    return result;
}

std::string RespGenerator::generate_resp_error(const RespError &input) const
{
    return fmt::format("-{}\r\n", input);
}

std::string RespGenerator::generate_resp_int(const RespInt &input) const
{
    return fmt::format(":{}\r\n", input);
}

std::string RespGenerator::generate_resp_null(const RespNull &input) const
{
    return "_\r\n";
}

RespGenerator::RespGenerator(std::shared_ptr<spdlog::logger> logger)
    : m_logger{std::move(logger)}
{
}

std::string RespGenerator::generate(const RespType &input) const
{
    if (std::holds_alternative<RespString>(input))
    {
        return generate_resp_string(std::get<RespString>(input));
    }
    else if (std::holds_alternative<RespArray>(input))
    {
        return generate_resp_array(std::get<RespArray>(input));
    }
    else if (std::holds_alternative<RespError>(input))
    {
        return generate_resp_error(std::get<RespError>(input));
    }
    else if (std::holds_alternative<RespInt>(input))
    {
        return generate_resp_int(std::get<RespInt>(input));
    }
    else if (std::holds_alternative<RespNull>(input))
    {
        return generate_resp_null(std::get<RespNull>(input));
    }
    return std::string{};
}

std::string RespGenerator::generate(std::shared_ptr<RespType> input) const
{
    if (!input)
    {
        m_logger->error("RespGenerator: Input is nullptr");
        return std::string{};
    }
    return generate(*input);
}

}   // namespace kredis