#include "Tokenizer.hpp"
#include <stack>
#include <iomanip>
#include <iostream>

namespace kjson
{
std::string token_type_to_string(TokenType type)
{
    switch (type)
    {
        case TokenType::opening_braces: return "OPEN_BRACE";
        case TokenType::closing_braces: return "CLOSE_BRACE";
        case TokenType::opening_square_brace: return "OPEN_SQUARE_BRACE";
        case TokenType::closing_square_brace: return "CLOSE_SQUARE_BRACE";
        case TokenType::dbl_quote: return "DOUBLE_QUOTE";
        case TokenType::colon: return "COLON";
        case TokenType::comma: return "COMMA";
        case TokenType::whitespace: return "WHITESPACE_VALUE";
        case TokenType::string_value: return "STRING_VALUE";
        case TokenType::boolean_value: return "BOOLEAN_VALUE";
        case TokenType::numeric_value: return "NUMERIC_VALUE";
        case TokenType::null_value: return "NULL_VALUE";
    }
    return "UNKNOWN";
}

Token::Token(TokenType type, std::string value)
    : m_type{type}, m_value{std::move(value)}
{
}

std::ostream& operator<<(std::ostream& os, const Token& token)
{
    os << "(Type: " << token_type_to_string(token.m_type) << ", Value: ";
    for (char c : token.m_value)
    {
        if (std::isspace(c))
        {
            os << std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(c) << std::dec;
        }
        else
        {
            os << c;
        }
    }
    os << ")";
    return os;
}

bool Tokenizer::handle_string(std::istream& is, char first_char, std::string& string_token)
{
    char c;
    while (is.get(c))
    {
        if (c == '"')
        {
            return true;
        }
        // Case when whitespace is not allowed in string except space (0x20)
        if (c == 0x09 || c == 0x0A || c == 0x09)
        {
            return false;
        }
        if (c == '\\')
        {
            char next_char;
            // Escape character encountered
            if (!(is.get(next_char)))
            {
                return false;
            }

            // Case of '\0'
            if (!(next_char == '"' || next_char == '\\' || next_char == '/'
                || next_char == 'b' || next_char == 'f' || next_char == 'n'
                || next_char == 'r' || next_char == 't' || next_char == 'u'))
            {
                return false;
            }

            string_token += c;
            string_token += next_char;

            if (next_char == 'u')
            {
                int counter = 0;
                char unicode_hex_char;

                while (counter < 4)
                {
                    if (!(is.get(unicode_hex_char)))
                    {
                        return false;
                    }
                    if (isxdigit(unicode_hex_char) == 0)
                    {
                        return false;
                    }
                    string_token += unicode_hex_char;
                    counter++;
                }
            }
        }
        else
        {
            string_token += c;
        }
    }
    return false;
}

bool Tokenizer::handle_integer(std::istream& is, char first_char, std::string& string_token, char& last_read_char)
{
    char c;
    if (first_char == '-')
    {
        string_token += first_char;
        while (is.get(c))
        {
            if (c == '0')
            {
                string_token += c;
                if (!(is.get(c)))
                {
                    return false;
                }
                last_read_char = c;
                break;
            }
            else if (std::isdigit(c) == 0)
            {
                last_read_char = c;
                break;
            }
            string_token += c;
        }
    }
    else if (first_char >= '1' && first_char <= '9')
    {
        string_token += first_char;
        while (is.get(c))
        {
            if (std::isdigit(c) == 0)
            {
                last_read_char = c;
                break;
            }
            string_token += c;
        }
    }
    else if (first_char == '0')
    {
        string_token += first_char;
        if (!(is.get(c)))
        {
            return false;
        }
        last_read_char = c;
    }
    else
    {
        return false;
    }
    return true;
}

bool Tokenizer::handle_fraction(std::istream& is, char first_char, std::string& string_token, char& last_read_char)
{
    if (first_char == 'e' || first_char == 'E')
    {
        // Fractional part absent, exponential part present
        last_read_char = first_char;
        return true;
    }
    if (first_char != '.')
    {
        // If '.' is missing, means fractional and exponential parts are absent, ignore it
        last_read_char = first_char;
        return true;
    }
    string_token += first_char;

    char c;
    while (is.get(c))
    {
        if (std::isdigit(c) == 0)
        {
            last_read_char = c;
            break;
        }
        string_token += c;
    }
    return true;
}

bool Tokenizer::handle_exponent(std::istream& is, char first_char, std::string& string_token, char& last_read_char)
{
    char c;
    if (first_char == 'e' || first_char == 'E')
    {
        string_token += first_char;
        last_read_char = first_char;
        bool sign_char_encountered = false;
        bool digit_encountered = false;

        while (is.get(c))
        {
            if (!sign_char_encountered && (c == '-' || c == '+'))
            {
                string_token += c;
                last_read_char = c;
                sign_char_encountered = true;
                continue;
            }
            if (sign_char_encountered && (c == '-' || c == '+'))
            {
                return false;
            }
            if (std::isdigit(c) == 0)
            {
                last_read_char = c;
                if ((!sign_char_encountered && !digit_encountered) || (sign_char_encountered && !digit_encountered))
                {
                    return false;
                }
                break;
            }
            if (!digit_encountered)
            {
                digit_encountered = true;
            }
            string_token += c;
            last_read_char = c;
        }
    }
    else
    {
        last_read_char = first_char;
    }
    return true;
}

bool Tokenizer::handle_numeric(std::istream& is, char first_char, std::string& string_token, char& last_read_char)
{
    return handle_integer(is, first_char, string_token, last_read_char)
        && handle_fraction(is, last_read_char, string_token, last_read_char)
        && handle_exponent(is, last_read_char, string_token, last_read_char);
}

bool Tokenizer::handle_boolean(std::istream& is, char first_char, std::string& string_token)
{
    string_token += first_char;

    char buf[5] = {0};
    if (first_char == 't')
    {
        if (!is.read(buf, 3))
        {
            return false;
        }
        std::string remaining{buf};
        if (remaining != "rue")
        {
            return false;
        }
        string_token += remaining;
    }
    else if (first_char == 'f')
    {
        if (!is.read(buf, 4))
        {
            return false;
        }
        std::string remaining{buf};
        if (remaining != "alse")
        {
            return false;
        }
        string_token += remaining;
    }
    else
    {
        return false;
    }
    return true;
}

bool Tokenizer::handle_null(std::istream& is, char first_char, std::string& string_token)
{
    string_token += first_char;

    char buf[5] = {0};
    if (first_char == 'n')
    {
        if (!is.read(buf, 3))
        {
            return false;
        }
        std::string remaining{buf};
        if (remaining != "ull")
        {
            return false;
        }
        string_token += remaining;
    }
    else
    {
        return false;
    }
    return true;
}

bool Tokenizer::handle_whitespace(std::istream& is, char first_char, std::string& string_token, char& last_read_char)
{
    string_token += first_char;

    char c;
    while (is.get(c))
    {
        if (c == 0x20 || c == 0x0A || c == 0x0D || c == 0x09)
        {
            string_token += c;
        }
        else
        {
            last_read_char = c;
            break;
        }
    }
    return true;
}

std::vector<Token> Tokenizer::tokenize(std::istream& input)
{
    std::vector<Token> tokens;
    char c;
    bool ignore_input = false;

    while (true)
    {
        if (!ignore_input && !input.get(c))
        {
            break;
        }
        if (ignore_input)
        {
            ignore_input = false;
        }
        // NUL character
        if (c == 0x00)
        {
            break;
        }
        switch (c)
        {
            case '{':
                tokens.emplace_back(TokenType::opening_braces, std::string{c});
                break;
            case '}':
                tokens.emplace_back(TokenType::closing_braces, std::string{c});
                break;
            case '[':
                tokens.emplace_back(TokenType::opening_square_brace, std::string{c});
                break;
            case ']':
                tokens.emplace_back(TokenType::closing_square_brace, std::string{c});
                break;
            case ',':
                tokens.emplace_back(TokenType::comma, std::string{c});
                break;
            case ':':
                tokens.emplace_back(TokenType::colon, std::string{c});
                break;
                break;
            case '"':
            {
                std::string string_token{};
                tokens.emplace_back(TokenType::dbl_quote, std::string{c});
                if (!handle_string(input, c, string_token))
                {
                    std::cerr << "Error during reading string value\n";
                    return std::vector<Token>{};
                }
                tokens.emplace_back(TokenType::string_value, string_token);
                tokens.emplace_back(TokenType::dbl_quote, std::string{c});
                break;
            }
            case 0x20:  // SPACE
            case 0x0A:  // LINE FEED (\n)
            case 0x0D:  // CARRIAGE RETURN (\r)
            case 0x09:  // HORIZONTAL TAB (\t)
            {
                std::string string_token{};
                char last_read_char{};
                if (!handle_whitespace(input, c, string_token, last_read_char))
                {
                    std::cerr << "Error during reading whitespace value\n";
                    return std::vector<Token>{};
                }
                tokens.emplace_back(TokenType::whitespace, string_token);
                ignore_input = true;
                c = last_read_char;
                break;
            }
            case '-':
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            {
                std::string string_token{};
                char last_read_char{};
                if (!handle_numeric(input, c, string_token, last_read_char))
                {
                    std::cerr << "Error during reading numeric value\n";
                    return std::vector<Token>{};
                }
                tokens.emplace_back(TokenType::numeric_value, string_token);
                ignore_input = true;
                c = last_read_char;
                break;
            }
            case 't':
            case 'f':
            {
                std::string string_token{};
                if (!handle_boolean(input, c, string_token))
                {
                    std::cerr << "Error during reading boolean value\n";
                    return std::vector<Token>{};
                }
                tokens.emplace_back(TokenType::boolean_value, string_token);
                break;
            }
            case 'n':
            {
                std::string string_token{};
                if (!handle_null(input, c, string_token))
                {
                    std::cerr << "Error during reading null value\n";
                    return std::vector<Token>{};
                }
                tokens.emplace_back(TokenType::null_value, string_token);
                break;
            }
            default:
                std::cerr << "Unknown character found " << std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(c) << std::dec << " (" << c << ")\n";
                return std::vector<Token>{};
        }
    }
    return tokens;
}
}   // namespace kjson