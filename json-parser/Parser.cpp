#include "Parser.hpp"

namespace kjson
{
Parser::Parser()
{
    RightSideTerminal whitespace = make_right_side_terminal(TokenType::whitespace, true);
    RightSideTerminal cstring = make_right_side_terminal(TokenType::string_value, true);
    RightSideTerminal comma = make_right_side_terminal(TokenType::comma, false);
    RightSideTerminal colon = make_right_side_terminal(TokenType::colon, false);
    RightSideTerminal number = make_right_side_terminal(TokenType::numeric_value, false);
    RightSideTerminal boolean = make_right_side_terminal(TokenType::boolean_value, false);
    RightSideTerminal nullv = make_right_side_terminal(TokenType::null_value, false);
    RightSideTerminal opening_brace = make_right_side_terminal(TokenType::opening_braces, false);
    RightSideTerminal closing_brace = make_right_side_terminal(TokenType::closing_braces, false);
    RightSideTerminal opening_sq_brace = make_right_side_terminal(TokenType::opening_square_brace, false);
    RightSideTerminal closing_sq_brace = make_right_side_terminal(TokenType::closing_square_brace, false);
    RightSideTerminal double_quote = make_right_side_terminal(TokenType::dbl_quote, false);

    m_rule_map[LeftSideNonTerminal::json] = {
        {LeftSideNonTerminal::element}
    };
    m_rule_map[LeftSideNonTerminal::element] = {
        {whitespace, LeftSideNonTerminal::value, whitespace}
    };
    m_rule_map[LeftSideNonTerminal::elements] = {
        {LeftSideNonTerminal::element},
        {LeftSideNonTerminal::element, comma, LeftSideNonTerminal::elements}
    };
    m_rule_map[LeftSideNonTerminal::value] = {
        {LeftSideNonTerminal::object},
        {LeftSideNonTerminal::array},
        {LeftSideNonTerminal::string},
        {nullv},
        {number},
        {boolean}
    };
    m_rule_map[LeftSideNonTerminal::object] = {
        {opening_brace, whitespace, closing_brace},
        {opening_brace, LeftSideNonTerminal::members, closing_brace}
    };
    m_rule_map[LeftSideNonTerminal::members] = {
        {LeftSideNonTerminal::member},
        {LeftSideNonTerminal::member, comma, LeftSideNonTerminal::members}
    };
    m_rule_map[LeftSideNonTerminal::member] = {
        {whitespace, LeftSideNonTerminal::string, whitespace, colon, LeftSideNonTerminal::element}
    };
    m_rule_map[LeftSideNonTerminal::array] = {
        {opening_sq_brace, whitespace, closing_sq_brace},
        {opening_sq_brace, LeftSideNonTerminal::elements, closing_sq_brace}
    };
    m_rule_map[LeftSideNonTerminal::string] = {
        {double_quote, cstring, double_quote}
    };
}

bool Parser::parse(const std::vector<Token> &tokens)
{
    return tokens[0].m_type == TokenType::opening_braces && tokens[1].m_type == TokenType::closing_braces;
}

RightSideTerminal make_right_side_terminal(TokenType type, bool has_epsilon_value)
{
    return RightSideTerminal{type, has_epsilon_value};
}
} // namespace kjson