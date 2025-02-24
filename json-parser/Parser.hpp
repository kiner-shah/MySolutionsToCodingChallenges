#pragma once

#include "Tokenizer.hpp"
#include <unordered_map>
#include <variant>
#include <optional>

namespace kjson
{
enum class LeftSideNonTerminal
{
    json,
    element,
    elements,
    members,
    member,
    value,
    object,
    array,
    string
};

struct RightSideTerminal
{
    TokenType m_token_type;
    bool has_epsilon_value; // True if the token type can have empty value, false otherwise
};

RightSideTerminal make_right_side_terminal(TokenType type, bool has_epsilon_value);

class Parser
{
    using Rule = std::vector<std::variant<LeftSideNonTerminal, RightSideTerminal>>;
    using RuleList = std::vector<Rule>;
    using RuleMap = std::unordered_map<LeftSideNonTerminal, RuleList>;

    RuleMap m_rule_map;

public:
    Parser();
    bool parse(const std::vector<Token>& tokens);
};
}   // namespace kjson