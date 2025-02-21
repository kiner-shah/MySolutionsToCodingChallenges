#include "Tokenizer.hpp"
#include <istream>
#include <ostream>
#include <stack>
#include <iomanip>

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
        case TokenType::character: return "CHAR";
        case TokenType::digit: return "DIGIT";
        case TokenType::whitespace: return "WHITESPACE";
        case TokenType::escape_char: return "ESCAPE_CHAR";
    }
    return "UNKNOWN";
}

Token::Token(TokenType type, char value)
    : m_type{type}, m_value{value}
{
}

std::ostream& operator<<(std::ostream& os, const Token& token)
{
    os << "(Type: " << token_type_to_string(token.m_type) << ", Value: ";
    if (std::isspace(token.m_value))
    {
        os << "0x" << std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(token.m_value) << std::dec;
    }
    else
    {
        os << '\'' << token.m_value << '\'';
    }
    os << ')';
    return os;
}

std::vector<Token> Tokenizer::tokenize(std::istream& input)
{
    std::vector<Token> tokens;
    char c;

    while (input.get(c))
    {
        switch (c)
        {
            case '{':
                tokens.emplace_back(TokenType::opening_braces, c);
                break;
            case '}':
                tokens.emplace_back(TokenType::closing_braces, c);
                break;
            case '[':
                tokens.emplace_back(TokenType::opening_square_brace, c);
                break;
            case ']':
                tokens.emplace_back(TokenType::closing_square_brace, c);
                break;
            case ',':
                tokens.emplace_back(TokenType::comma, c);
                break;
            case ':':
                tokens.emplace_back(TokenType::colon, c);
                break;
            case '\\':
                tokens.emplace_back(TokenType::escape_char, c);
                break;
            case '"':
                tokens.emplace_back(TokenType::dbl_quote, c);
                break;
            case 0x20:  // SPACE
            case 0x0A:  // LINE FEED (\n)
            case 0x0D:  // CARRIAGE RETURN (\r)
            case 0x09:  // HORIZONTAL TAB (\t)
                tokens.emplace_back(TokenType::whitespace, c);
                break;
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
                tokens.emplace_back(TokenType::digit, c);
                break;
            default:
                tokens.emplace_back(TokenType::character, c);
        }
    }
    return tokens;
}
}   // namespace kjson