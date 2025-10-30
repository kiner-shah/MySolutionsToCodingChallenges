#include "RespParser.hpp"
#include <cctype>

namespace kredis
{
RespParserState RespParser::parse_simple_string(std::string_view input, RespString& parse_output, std::size_t& parse_end_pos)
{
    if (input.empty() || !input.starts_with('+'))
    {
        m_logger->error("Simple string: input is either empty or doesn't start with '+': {}", input);
        return RespParserState::Invalid;
    }
    parse_end_pos = 1;

    auto content_end_pos = input.find("\r\n");
    if (content_end_pos == std::string_view::npos)
    {
        m_logger->error("Simple string: no CRLF found in input {}", input);
        return RespParserState::Incomplete;
    }
    parse_end_pos = content_end_pos + 2;

    auto content_length = content_end_pos - 1;
    auto content = input.substr(1, content_length);
    for (char c : content)
    {
        if (c == '\r' || c == '\n')
        {
            m_logger->error("Simple string: invalid character {} found in input {}", static_cast<int>(c), input);
            return RespParserState::Invalid;
        }
    }
    parse_output.m_str = std::string{content};
    parse_output.m_is_bulk = false;
    return RespParserState::Complete;
}

RespParserState RespParser::parse_bulk_string(std::string_view input, RespString& parse_output, std::size_t& parse_end_pos)
{
    if (input.empty() || !input.starts_with('$'))
    {
        m_logger->error("Bulk string: input is either empty or doesn't start with '$': {}", input);
        return RespParserState::Invalid;
    }
    parse_end_pos = 1;

    auto length_string_end_pos = input.find("\r\n");
    if (length_string_end_pos == std::string_view::npos)
    {
        m_logger->error("Bulk string: no CRLF found in input {}", input);
        return RespParserState::Incomplete;
    }
    parse_end_pos = length_string_end_pos + 2;

    auto length_string_len = length_string_end_pos - 1;
    auto length_string = input.substr(1, length_string_len);
    for (unsigned char c : length_string)
    {
        if (!std::isdigit(c))
        {
            m_logger->error("Bulk string: invalid length string {} found in input {}", length_string, input);
            return RespParserState::Invalid;
        }
    }
    auto content_end_pos = input.find("\r\n", length_string_end_pos + 2);
    if (content_end_pos == std::string_view::npos)
    {
        m_logger->error("Bulk string: no CRLF found in input after length string {}", input);
        return RespParserState::Incomplete;
    }
    parse_end_pos = content_end_pos + 2;

    auto content_len = content_end_pos - length_string_end_pos - 2;
    auto expected_content_len = std::stoull(std::string{length_string});
    if (content_len != expected_content_len)
    {
        m_logger->error("Bulk string: expected content length {} but got {}", expected_content_len, content_len);
        return RespParserState::Invalid;
    }
    auto content = input.substr(length_string_end_pos + 2, content_len);
    parse_output.m_str = std::string{content};
    parse_output.m_is_bulk = true;
    return RespParserState::Complete;
}

RespParserState RespParser::parse_error(std::string_view input, RespError& parse_output, std::size_t& parse_end_pos)
{
    if (input.empty() || !input.starts_with('-'))
    {
        m_logger->error("Error: input is either empty or doesn't start with '-': {}", input);
        return RespParserState::Invalid;
    }
    parse_end_pos = 1;

    auto content_end_pos = input.find("\r\n");
    if (content_end_pos == std::string_view::npos)
    {
        m_logger->error("Error: no CRLF found in input {}", input);
        return RespParserState::Incomplete;
    }
    parse_end_pos = content_end_pos + 2;

    auto content_length = content_end_pos - 1;
    auto content = input.substr(1, content_length);
    for (char c : content)
    {
        if (c == '\r' || c == '\n')
        {
            m_logger->error("Error: invalid character {} found in input {}", static_cast<int>(c), input);
            return RespParserState::Invalid;
        }
    }
    parse_output = std::string{content};
    return RespParserState::Complete;
}

RespParserState RespParser::parse_integer(std::string_view input, RespInt& parse_output, std::size_t& parse_end_pos)
{
    if (input.empty() || !input.starts_with(':'))
    {
        m_logger->error("Integer: input is either empty or doesn't start with ':': {}", input);
        return RespParserState::Invalid;
    }
    parse_end_pos = 1;

    auto content_end_pos = input.find("\r\n");
    if (content_end_pos == std::string_view::npos)
    {
        m_logger->error("Integer: no CRLF found in input {}", input);
        return RespParserState::Incomplete;
    }
    parse_end_pos = content_end_pos + 2;

    auto content_length = content_end_pos - 1;
    auto content = input.substr(1, content_length);
    if (content.empty())
    {
        m_logger->error("Integer: content is empty");
        return RespParserState::Invalid;
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
        return RespParserState::Invalid;
    }

    // Check rest of the content string
    for (std::size_t i = 1; i < content.length(); i++)
    {
        if (!std::isdigit(content[i]))
        {
            m_logger->error("Integer: invalid character {} found in input {}", static_cast<int>(content[i]), input);
            return RespParserState::Invalid;
        }
        integer_str += content[i];
    }
    parse_output = std::stoll(integer_str);
    if (!is_positive)
    {
        parse_output *= -1;
    }
    return RespParserState::Complete;
}

RespParserState RespParser::parse_array(std::string_view input, RespArray& parse_output, std::size_t& parse_end_pos)
{
    if (input.empty() || !input.starts_with('*'))
    {
        m_logger->error("Array: input is either empty or doesn't start with '*': {}", input);
        return RespParserState::Invalid;
    }
    parse_end_pos = 1;

    auto length_string_end_pos = input.find("\r\n");
    if (length_string_end_pos == std::string_view::npos)
    {
        m_logger->error("Array: no CRLF found in input {}", input);
        return RespParserState::Incomplete;
    }
    parse_end_pos = length_string_end_pos + 2;

    auto length_string_len = length_string_end_pos - 1;
    auto length_string = input.substr(1, length_string_len);
    if (length_string.empty())
    {
        m_logger->error("Array: length string is empty in input {}", input);
        return RespParserState::Invalid;
    }
    for (unsigned char c : length_string)
    {
        if (!std::isdigit(c))
        {
            m_logger->error("Array: invalid character {} found in input {}", static_cast<int>(c), input);
            return RespParserState::Invalid;
        }
    }
    auto array_len = std::stoll(std::string{length_string});
    auto counter = array_len;
    auto start_pos = length_string_end_pos + 2;
    std::size_t last_end_pos{};
    while (counter > 0)
    {
        if (start_pos >= input.size())
        {
            break;
        }
        auto element = input.substr(start_pos);
        std::size_t end_pos{};
        RespParserState state;
        switch (input[start_pos])
        {
            case '$':
            {
                RespNull resp_null;
                RespString resp_bulk_string;
                state = parse_null(element, resp_null, end_pos);
                if (state == RespParserState::Complete)
                {
                    parse_output.push_back(resp_null);
                }
                else if (state == RespParserState::Invalid)
                {
                    if ((state = parse_bulk_string(element, resp_bulk_string, end_pos)) == RespParserState::Complete)
                    {
                        parse_output.push_back(resp_bulk_string);
                    }
                }
                break;
            }
            case ':':
            {
                RespInt resp_int;
                if ((state = parse_integer(element, resp_int, end_pos)) == RespParserState::Complete)
                {
                    parse_output.push_back(resp_int);
                }
                break;
            }
            case '+':
            {
                RespString resp_string;
                if ((state = parse_simple_string(element, resp_string, end_pos)) == RespParserState::Complete)
                {
                    parse_output.push_back(resp_string);
                }
                break;
            }
            case '-':
            {
                RespError resp_error;
                if ((state = parse_error(element, resp_error, end_pos)) == RespParserState::Complete)
                {
                    parse_output.push_back(resp_error);
                }
                break;
            }
            case '*':
            {
                RespArray resp_array;
                if ((state = parse_array(element, resp_array, end_pos)) == RespParserState::Complete)
                {
                    parse_output.push_back(resp_array);
                }
                break;
            }
            default:
            {
                m_logger->error("Array: invalid element type {} in input {}", input[start_pos], input);
                state = RespParserState::Invalid;
                break;
            }
        }
        auto prev_start_pos = start_pos;
        start_pos = prev_start_pos + end_pos;
        last_end_pos = prev_start_pos + end_pos;
        parse_end_pos = last_end_pos;
        counter--;
        if (state != RespParserState::Complete)
        {
            return state;
        }
    }
    if (counter != 0)
    {
        m_logger->error("Array: input ended unexpectedly, {} elements remaining", counter);
        return RespParserState::Incomplete;
    }
    return RespParserState::Complete;
}

RespParserState RespParser::check_null_variant(std::string_view input, std::string_view null_variant, std::size_t &parse_end_pos)
{
    std::size_t last_processed_index = 0;
    std::size_t index = 0;
    RespParserState state {RespParserState::Complete};
    for (; index < null_variant.size() && last_processed_index < input.size(); index++, last_processed_index++)
    {
        if (input[index] != null_variant[index])
        {
            state = RespParserState::Invalid;
            break;
        }
    }
    if (index < null_variant.size())
    {
        if (last_processed_index == input.size())
        {
            // Input fully processed, but null variant processing was partial
            // Means input needs more data
            state = RespParserState::Incomplete;
        }
    }
    parse_end_pos = last_processed_index;
    return state;
}

RespParserState RespParser::parse_null(std::string_view input, RespNull& parse_output, std::size_t& parse_end_pos)
{
    static constexpr const char* null_variant1 = "$-1\r\n";
    static constexpr const char* null_variant2 = "*-1\r\n";
    static constexpr const char* null_variant3 = "_\r\n";

    if (input.empty())
    {
        m_logger->error("Null: input is empty", input);
        return RespParserState::Invalid;
    }
    parse_end_pos = 0;

    m_logger->debug("Null: checking different variants");
    auto state = check_null_variant(input, null_variant1, parse_end_pos);
    if (state == RespParserState::Invalid)
    {
        m_logger->trace("Null: match with null variant1 failed, trying with null variant2");
        state = check_null_variant(input, null_variant2, parse_end_pos);
        if (state == RespParserState::Invalid)
        {
            m_logger->trace("Null: match with null variant2 failed, trying with null variant3");
            state = check_null_variant(input, null_variant3, parse_end_pos);
            if (state == RespParserState::Invalid)
            {
                m_logger->trace("Null: match with null variant3 failed, input is invalid: {}", input);
            }
        }
    }

    return state;
}

RespParser::RespParser(std::shared_ptr<spdlog::logger> logger)
    : m_logger{std::move(logger)}
{
}

bool RespParser::parse(std::string_view input, std::vector<RespParserResult> &result)
{
    if (input.empty())
    {
        return false;
    }

    std::string_view original_input = input;
    std::size_t parse_end_pos{};
    std::size_t parse_start_pos{0};
    RespParserState state{};
    while (parse_start_pos < original_input.size())
    {
        switch (original_input[parse_start_pos])
        {
            case '$':
            {
                RespString resp_bulk_string;
                RespNull resp_null;
                state = parse_null(input, resp_null, parse_end_pos);
                if (state == RespParserState::Complete)
                {
                    result.emplace_back(resp_null, state);
                }
                else if (state == RespParserState::Invalid)
                {
                    if ((state = parse_bulk_string(input, resp_bulk_string, parse_end_pos)) == RespParserState::Complete)
                    {
                        result.emplace_back(resp_bulk_string, state);
                    }
                }
                break;
            }
            case '*':
            {
                RespArray resp_array;
                RespNull resp_null;
                state = parse_null(input, resp_null, parse_end_pos);
                if (state == RespParserState::Complete)
                {
                    result.emplace_back(resp_null, state);
                }
                else if (state == RespParserState::Invalid)
                {
                    if ((state = parse_array(input, resp_array, parse_end_pos)) == RespParserState::Complete)
                    {
                        result.emplace_back(resp_array, state);
                    }
                }
                break;
            }
            case ':':
            {
                RespInt resp_int;
                if ((state = parse_integer(input, resp_int, parse_end_pos)) == RespParserState::Complete)
                {
                    result.emplace_back(resp_int, state);
                }
                break;
            }
            case '+':
            {
                RespString resp_simple_string;
                if ((state = parse_simple_string(input, resp_simple_string, parse_end_pos)) == RespParserState::Complete)
                {
                    result.emplace_back(resp_simple_string, state);
                }
                break;
            }
            case '-':
            {
                RespError resp_error;
                if ((state = parse_error(input, resp_error, parse_end_pos)) == RespParserState::Complete)
                {
                    result.emplace_back(resp_error, state);
                }
                break;
            }
            default:
                m_logger->error("Invalid first character {} in input {}", input[0], input);
                result.emplace_back(std::nullopt, RespParserState::Invalid);
                return false;
        }
        if (state != RespParserState::Complete)
        {
            result.emplace_back(std::nullopt, state);
            if (state == RespParserState::Incomplete)
            {
                return false;
            }
        }
        parse_start_pos = parse_start_pos + parse_end_pos;
        input = original_input.substr(parse_start_pos);
    }
    m_logger->debug("Parse completed, parse ended at position {}, input size {}", parse_end_pos, original_input.size());
    return true;
}

std::ostream &operator<<(std::ostream &os, const RespString &resp_string)
{
    os << "{String: " << resp_string.m_str << ", IsBulk: " << std::boolalpha << resp_string.m_is_bulk << '}' << std::noboolalpha;
    return os;
}

std::string resp_parser_state_to_string(RespParserState state)
{
    switch (state)
    {
        case RespParserState::Complete: return "Complete";
        case RespParserState::Incomplete: return "Incomplete";
        case RespParserState::Invalid: return "Invalid";
        default: return "UNKNOWN_STATE";
    }
    return "UNKNOWN_STATE";
}
} // namespace kredis
