#include "Parser.hpp"
#include <iostream>

namespace kjson
{
std::string left_side_non_terminal_to_string(LeftSideNonTerminal non_terminal)
{
    switch (non_terminal)
    {
        case LeftSideNonTerminal::array: return "ARRAY";
        case LeftSideNonTerminal::element: return "ELEMENT";
        case LeftSideNonTerminal::elements: return "ELEMENTS";
        case LeftSideNonTerminal::member: return "MEMBER";
        case LeftSideNonTerminal::members: return "MEMBERS";
        case LeftSideNonTerminal::json: return "JSON";
        case LeftSideNonTerminal::object: return "OBJECT";
        case LeftSideNonTerminal::string :return "STRING";
        case LeftSideNonTerminal::value: return "VALUE";
    }
    return "UNKNOWN";
}

std::ostream& operator<<(std::ostream& os, const RightSideTerminal& terminal)
{
    os << "(Token type: " << token_type_to_string(terminal.m_token_type) << ", Has epsilon?: " << terminal.has_epsilon_value << ')';
    return os;
}

void Parser::print_rule(LeftSideNonTerminal non_terminal, const Rule& rule) const
{
    std::cout << "Rule: " << left_side_non_terminal_to_string(non_terminal) << " -> ";
    for (const auto& element : rule)
    {
        if (std::holds_alternative<LeftSideNonTerminal>(element))
        {
            std::cout << left_side_non_terminal_to_string(std::get<LeftSideNonTerminal>(element)) << ' ';
        }
        else
        {
            auto terminal = std::get<RightSideTerminal>(element);
            std::cout << token_type_to_string(terminal.m_token_type) << ' ';
        }
    }
    std::cout << '\n';
}

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
        {whitespace, LeftSideNonTerminal::object, whitespace},
        {whitespace, LeftSideNonTerminal::array, whitespace}
    };
    m_rule_map[LeftSideNonTerminal::element] = {
        {whitespace, LeftSideNonTerminal::value, whitespace}
    };
    m_rule_map[LeftSideNonTerminal::elements] = {
        {LeftSideNonTerminal::element, comma, LeftSideNonTerminal::elements},
        {LeftSideNonTerminal::element}
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
        {opening_brace, LeftSideNonTerminal::members, closing_brace},
        {opening_brace, whitespace, closing_brace}
    };
    m_rule_map[LeftSideNonTerminal::members] = {
        {LeftSideNonTerminal::member, comma, LeftSideNonTerminal::members},
        {LeftSideNonTerminal::member}
    };
    m_rule_map[LeftSideNonTerminal::member] = {
        {whitespace, LeftSideNonTerminal::string, whitespace, colon, LeftSideNonTerminal::element}
    };
    m_rule_map[LeftSideNonTerminal::array] = {
        {opening_sq_brace, LeftSideNonTerminal::elements, closing_sq_brace},
        {opening_sq_brace, whitespace, closing_sq_brace}
    };
    m_rule_map[LeftSideNonTerminal::string] = {
        {double_quote, cstring, double_quote}
    };
}

bool Parser::parse_json(const std::vector<Token> &tokens, size_t& token_index, size_t indent_level, LeftSideNonTerminal non_terminal)
{
    // std::cout << "Processing rules for " << left_side_non_terminal_to_string(non_terminal) << " Token Index: " << token_index << '\n';

    if (token_index == tokens.size())
    {
        return false;
    }

    // Case when nesting is 20 or more levels deep
    // Indent level for nesting level 19 is 77
    if (indent_level > 77)
    {
        std::cerr << "Too deeply nested structure found\n";
        return false;
    }

    unsigned int unmatched_rule_count = 0;
    for (Rule rule : m_rule_map[non_terminal])
    {
        size_t current_element_index = 0;
        bool current_rule_not_matched = false;
        size_t token_index_copy = token_index;

        // std::cout << indent_level << ' ';
        // for (size_t i = 0; i < indent_level; i++)
        // {
        //     std::cout << '.';
        // }
        // print_rule(non_terminal, rule);
        for (const auto& element : rule)
        {
            if (std::holds_alternative<LeftSideNonTerminal>(element))
            {
                // Expand non-terminal
                LeftSideNonTerminal rule_non_terminal = std::get<LeftSideNonTerminal>(element);
                //std::cout << "Processing non-terminal " << left_side_non_terminal_to_string(rule_non_terminal) << '\n';
                if (!parse_json(tokens, token_index_copy, indent_level + 1, rule_non_terminal))
                {
                    // std::cerr << "Unmatched (Non-Terminal) " << left_side_non_terminal_to_string(rule_non_terminal) << '\n';
                    current_rule_not_matched = true;
                    break;
                }
                else
                {
                    //std::cout << "Matched (Non-Terminal) " << left_side_non_terminal_to_string(rule_non_terminal) << '\n';
                }
            }
            else
            {
                RightSideTerminal rule_terminal = std::get<RightSideTerminal>(element);
                //std::cout << "Processing terminal " << rule_terminal << " Token " << tokens[token_index_copy] << '\n';
                if (token_index_copy < tokens.size() && tokens[token_index_copy].m_type == rule_terminal.m_token_type)
                {
                    // Matched, increment token index
                    //std::cout << "Matched (Terminal) " << rule_terminal << '\n';
                    token_index_copy++;
                }
                else if (rule_terminal.has_epsilon_value)
                {
                    // Check next element
                    //std::cout << "Skipping current element " << rule_terminal << '\n';
                    continue;
                }
                else
                {
                    // std::cerr << "Unmatched (Terminal) " << tokens[token_index_copy] << ' ' << rule_terminal << '\n';
                    current_rule_not_matched = true;
                    break;
                }
            }
            current_element_index++;
        }
        if (current_rule_not_matched)
        {
            unmatched_rule_count++;
        }
        else
        {
            // Rule matched
            token_index = token_index_copy;
            break;
        }
    }
    if (unmatched_rule_count == m_rule_map[non_terminal].size())
    {
        // std::cerr << "Failed to parse for " << left_side_non_terminal_to_string(non_terminal) << '\n';
        return false;
    }
    // std::cout << "Success parsing " << left_side_non_terminal_to_string(non_terminal) << ' ' << token_index << '\n';
    return true;
}

bool Parser::parse(const std::vector<Token> &tokens)
{
    if (tokens.empty())
    {
        return false;
    }
    size_t start_token_index = 0;
    bool result = parse_json(tokens, start_token_index, 0, LeftSideNonTerminal::json);
    // Case when there are extra tokens after the actual JSON structure
    if (result && start_token_index < tokens.size())
    {
        return false;
    }
    return result;
}

RightSideTerminal make_right_side_terminal(TokenType type, bool has_epsilon_value)
{
    return RightSideTerminal{type, has_epsilon_value};
}
} // namespace kjson