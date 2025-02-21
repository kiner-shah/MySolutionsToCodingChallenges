#pragma once

#include <string>
#include <vector>

namespace kjson
{

enum class TokenType
{
    opening_braces,
    closing_braces,
    opening_square_brace,
    closing_square_brace,
    dbl_quote,
    colon,
    comma,
    character,
    digit,
    whitespace,
    escape_char,
    string_value,
    boolean_value,
    numeric_value,
    null_value
};

std::string token_type_to_string(TokenType type);

struct Token
{
    TokenType m_type;
    char m_value;

    Token(TokenType type, char value);
    friend std::ostream& operator<<(std::ostream& os, const Token& token);
};

class Tokenizer
{
public:
    std::vector<Token> tokenize(std::istream& input);
};
}   // namespace kjson