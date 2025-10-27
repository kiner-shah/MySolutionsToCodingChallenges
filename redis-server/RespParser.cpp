#include "RespParser.hpp"
#include <cctype>

namespace kredis
{
bool RespParser::parse_simple_string(std::string_view input, RespString& parse_output, std::size_t& parse_end_pos)
{
    if (input.empty() || !input.starts_with('+'))
    {
        m_logger->error("Simple string: input is either empty or doesn't start with '+': {}", input);
        return false;
    }
    auto content_end_pos = input.find("\r\n");
    if (content_end_pos == std::string_view::npos)
    {
        m_logger->error("Simple string: no CRLF found in input {}", input);
        return false;
    }
    auto content_length = content_end_pos - 1;
    auto content = input.substr(1, content_length);
    for (char c : content)
    {
        if (c == '\r' || c == '\n')
        {
            m_logger->error("Simple string: invalid character {} found in input {}", static_cast<int>(c), input);
            return false;
        }
    }
    parse_end_pos = content_end_pos + 2;
    parse_output.m_str = std::string{content};
    parse_output.m_is_bulk = false;
    return true;
}

bool RespParser::parse_bulk_string(std::string_view input, RespString& parse_output, std::size_t& parse_end_pos)
{
    if (input.empty() || !input.starts_with('$'))
    {
        m_logger->error("Bulk string: input is either empty or doesn't start with '$': {}", input);
        return false;
    }
    auto length_string_end_pos = input.find("\r\n");
    if (length_string_end_pos == std::string_view::npos)
    {
        m_logger->error("Bulk string: no CRLF found in input {}", input);
        return false;
    }
    auto length_string_len = length_string_end_pos - 1;
    auto length_string = input.substr(1, length_string_len);
    for (unsigned char c : length_string)
    {
        if (!std::isdigit(c))
        {
            m_logger->error("Bulk string: invalid length string {} found in input {}", length_string, input);
            return false;
        }
    }
    auto content_end_pos = input.find("\r\n", length_string_end_pos + 2);
    if (content_end_pos == std::string_view::npos)
    {
        m_logger->error("Bulk string: no CRLF found in input after length string {}", input);
        return false;
    }
    auto content_len = content_end_pos - length_string_end_pos - 2;
    auto expected_content_len = std::stoull(std::string{length_string});
    if (content_len != expected_content_len)
    {
        m_logger->error("Bulk string: expected content length {} but got {}", expected_content_len, content_len);
        return false;
    }
    auto content = input.substr(length_string_end_pos + 2, content_len);
    parse_end_pos = content_end_pos + 2;
    parse_output.m_str = std::string{content};
    parse_output.m_is_bulk = true;
    return true;
}

bool RespParser::parse_error(std::string_view input, RespError& parse_output, std::size_t& parse_end_pos)
{
    if (input.empty() || !input.starts_with('-'))
    {
        m_logger->error("Error: input is either empty or doesn't start with '-': {}", input);
        return false;
    }
    auto content_end_pos = input.find("\r\n");
    if (content_end_pos == std::string_view::npos)
    {
        m_logger->error("Error: no CRLF found in input {}", input);
        return false;
    }
    auto content_length = content_end_pos - 1;
    auto content = input.substr(1, content_length);
    for (char c : content)
    {
        if (c == '\r' || c == '\n')
        {
            m_logger->error("Error: invalid character {} found in input {}", static_cast<int>(c), input);
            return false;
        }
    }
    parse_end_pos = content_end_pos + 2;
    parse_output = std::string{content};
    return true;
}

bool RespParser::parse_integer(std::string_view input, RespInt& parse_output, std::size_t& parse_end_pos)
{
    if (input.empty() || !input.starts_with(':'))
    {
        m_logger->error("Integer: input is either empty or doesn't start with ':': {}", input);
        return false;
    }
    auto content_end_pos = input.find("\r\n");
    if (content_end_pos == std::string_view::npos)
    {
        m_logger->error("Integer: no CRLF found in input {}", input);
        return false;
    }
    auto content_length = content_end_pos - 1;
    auto content = input.substr(1, content_length);
    if (content.empty())
    {
        m_logger->error("Integer: content is empty");
        return false;
    }

    // Check if first character is - or + or digit
    bool is_positive = true;
    std::string integer_str{};
    if (content[0] == '-' || content[0] == '+')
    {
        is_positive = content[0] == '+';
    }
    else if (std::isdigit(content[0]))
    {
        integer_str += content[0];
    }
    else
    {
        m_logger->error("Integer: unexpected first character {} found in input {}", content[0], input);
        return false;
    }

    // Check rest of the content string
    for (std::size_t i = 1; i < content.length(); i++)
    {
        if (!std::isdigit(content[i]))
        {
            m_logger->error("Integer: invalid character {} found in input {}", static_cast<int>(content[i]), input);
            return false;
        }
        integer_str += content[i];
    }
    parse_output = std::stoll(integer_str);
    if (!is_positive)
    {
        parse_output *= -1;
    }
    parse_end_pos = content_end_pos + 2;
    return true;
}

bool RespParser::parse_array(std::string_view input, RespArray& parse_output, std::size_t& parse_end_pos)
{
    if (input.empty() || !input.starts_with('*'))
    {
        m_logger->error("Array: input is either empty or doesn't start with '*': {}", input);
        return false;
    }
    auto length_string_end_pos = input.find("\r\n");
    auto length_string_len = length_string_end_pos - 1;
    auto length_string = input.substr(1, length_string_len);
    if (length_string.empty())
    {
        m_logger->error("Array: length string is empty in input {}", input);
        return false;
    }
    for (unsigned char c : length_string)
    {
        if (!std::isdigit(c))
        {
            m_logger->error("Array: invalid character {} found in input {}", static_cast<int>(c), input);
            return false;
        }
    }
    auto array_len = std::stoll(std::string{length_string});
    auto counter = array_len;
    auto start_pos = length_string_end_pos + 2;
    std::size_t last_end_pos{};
    while (true)
    {
        if (start_pos >= input.size())
        {
            break;
        }
        auto element = input.substr(start_pos);
        std::size_t end_pos{};
        switch (input[start_pos])
        {
            case '$':
            {
                RespNull resp_null;
                RespString resp_bulk_string;
                if (parse_null(element, resp_null, end_pos))
                {
                    parse_output.push_back(resp_null);
                }
                else if (parse_bulk_string(element, resp_bulk_string, end_pos))
                {
                    parse_output.push_back(resp_bulk_string);
                }
                else
                {
                    m_logger->error("Array: invalid element encountered at start_pos {} in input {}", start_pos, input);
                    return false;
                }
                break;
            }
            case ':':
            {
                RespInt resp_int;
                if (parse_integer(element, resp_int, end_pos))
                {
                    parse_output.push_back(resp_int);
                }
                else
                {
                    m_logger->error("Array: invalid element encountered at start_pos {} in input {}", start_pos, input);
                    return false;
                }
                break;
            }
            case '+':
            {
                RespString resp_string;
                if (parse_simple_string(element, resp_string, end_pos))
                {
                    parse_output.push_back(resp_string);
                }
                else
                {
                    m_logger->error("Array: invalid element encountered at start_pos {} in input {}", start_pos, input);
                    return false;
                }
                break;
            }
            default:
                m_logger->error("Array: invalid element type {} in input {}", input[start_pos], input);
                return false;
        }
        auto prev_start_pos = start_pos;
        start_pos = prev_start_pos + end_pos;
        last_end_pos = prev_start_pos + end_pos;
        counter--;
    }
    if (counter != 0)
    {
        m_logger->error("Array: input ended unexpectedly, {} elements remaining", counter);
        return false;
    }
    parse_end_pos = last_end_pos;
    return true;
}

bool RespParser::parse_null(std::string_view input, RespNull& parse_output, std::size_t& parse_end_pos)
{
    static const std::string null_variant1 = "$-1\r\n";
    static const std::string null_variant2 = "*-1\r\n";
    static const std::string null_variant3 = "_\r\n";
    if (input == null_variant1 || input == null_variant2 || input == null_variant3)
    {
        parse_output = nullptr;
        parse_end_pos = input.size();
        return true;
    }
    return false;
}

RespParser::RespParser(std::shared_ptr<spdlog::logger> logger)
    : m_logger{std::move(logger)}
{
}

bool RespParser::parse(std::string_view input, RespType &result)
{
    if (input.empty())
    {
        return false;
    }

    [[maybe_unused]] std::size_t parse_end_pos{};
    switch (input[0])
    {
        case '$':
        {
            RespString resp_bulk_string;
            RespNull resp_null;
            if (parse_null(input, resp_null, parse_end_pos))
            {
                result = resp_null;
            }
            else if (parse_bulk_string(input, resp_bulk_string, parse_end_pos))
            {
                result = resp_bulk_string;
            }
            else
            {
                return false;
            }
            break;
        }
        case '*':
        {
            RespArray resp_array;
            RespNull resp_null;
            if (parse_null(input, resp_null, parse_end_pos))
            {
                result = resp_null;
            }
            else if (parse_array(input, resp_array, parse_end_pos))
            {
                result = resp_array;
            }
            else
            {
                return false;
            }
            break;
        }
        case ':':
        {
            RespInt resp_int;
            if (!parse_integer(input, resp_int, parse_end_pos))
            {
                return false;
            }
            result = resp_int;
            break;
        }
        case '+':
        {
            RespString resp_simple_string;
            if (!parse_simple_string(input, resp_simple_string, parse_end_pos))
            {
                return false;
            }
            result = resp_simple_string;
            break;
        }
        case '-':
        {
            RespError resp_error;
            if (!parse_error(input, resp_error, parse_end_pos))
            {
                return false;
            }
            result = resp_error;
            break;
        }
        default:
            m_logger->error("Invalid first character {} in input {}", input[0], input);
            return false;
    }
    m_logger->debug("Parse successful, parse ended at position {}, input size {}", parse_end_pos, input.size());
    return true;
}

std::ostream &operator<<(std::ostream &os, const RespString &resp_string)
{
    os << "{String: " << resp_string.m_str << ", IsBulk: " << std::boolalpha << resp_string.m_is_bulk << '}' << std::noboolalpha;
    return os;
}
} // namespace kredis
