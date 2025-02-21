#include "Parser.hpp"

namespace kjson
{
bool Parser::parse(const std::vector<Token> &tokens)
{
    return tokens[0].m_type == TokenType::opening_braces && tokens[1].m_type == TokenType::closing_braces;
}
}   // namespace kjson