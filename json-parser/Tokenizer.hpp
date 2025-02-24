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
    whitespace,
    string_value,
    boolean_value,
    numeric_value,
    null_value
};

std::string token_type_to_string(TokenType type);

struct Token
{
    TokenType m_type;
    std::string m_value;

    Token(TokenType type, std::string value);
    friend std::ostream& operator<<(std::ostream& os, const Token& token);
};

class Tokenizer
{
    [[nodiscard]] bool handle_string(std::istream& is, char first_char, std::string& string_token);
    [[nodiscard]] bool handle_numeric(std::istream& is, char first_char, std::string& string_token, char& last_read_char);
    [[nodiscard]] bool handle_integer(std::istream& is, char first_char, std::string& string_token, char& last_read_char);
    [[nodiscard]] bool handle_fraction(std::istream& is, char first_char, std::string& string_token, char& last_read_char);
    [[nodiscard]] bool handle_exponent(std::istream& is, char first_char, std::string& string_token, char& last_read_char);
    [[nodiscard]] bool handle_boolean(std::istream& is, char first_char, std::string& string_token);
    [[nodiscard]] bool handle_null(std::istream& is, char first_char, std::string& string_token);

public:
    std::vector<Token> tokenize(std::istream& input);
};
}   // namespace kjson