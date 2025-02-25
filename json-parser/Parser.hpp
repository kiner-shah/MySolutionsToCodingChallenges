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

std::string left_side_non_terminal_to_string(LeftSideNonTerminal non_terminal);

struct RightSideTerminal
{
    TokenType m_token_type;
    bool has_epsilon_value; // True if the token type can have empty value, false otherwise

    friend std::ostream& operator<<(std::ostream& os, const RightSideTerminal& terminal);
};

RightSideTerminal make_right_side_terminal(TokenType type, bool has_epsilon_value);

class Parser
{
    using Rule = std::vector<std::variant<LeftSideNonTerminal, RightSideTerminal>>;
    using RuleList = std::vector<Rule>;
    using RuleMap = std::unordered_map<LeftSideNonTerminal, RuleList>;

    RuleMap m_rule_map;

    void print_rule(LeftSideNonTerminal non_terminal, const Rule& rule) const;
    [[nodiscard]] bool parse_json(const std::vector<Token>& tokens, size_t& token_index, size_t indent_level, LeftSideNonTerminal non_terminal);

public:
    Parser();
    [[nodiscard]] bool parse(const std::vector<Token>& tokens);
};
}   // namespace kjson