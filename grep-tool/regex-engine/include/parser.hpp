#ifndef K_REGIX_ENGINE_PARSER_HPP
#define K_REGIX_ENGINE_PARSER_HPP

#include <functional>
#include <string_view>
#include <vector>
#include "parser_types.hpp"

namespace kregex
{
template<typename T>
struct ParseValue
{
    T value;
    std::string_view remaining_regex;

    bool operator==(const ParseValue& other) const
    {
        return value == other.value && remaining_regex == other.remaining_regex;
    }
    bool operator!=(const ParseValue& other) const
    {
        return !(*this == other);
    }
};

using ParseError = std::string;

template<typename T>
using ParseResult = std::variant<ParseValue<T>, ParseError>;

ParseResult<Integer> parse_number(std::string_view regex);
ParseResult<Letters> parse_letters(std::string_view regex);
ParseResult<Quantifier> parse_quantifier(std::string_view regex);
ParseResult<RangeQuantifier> parse_range_quantifier(std::string_view regex);
ParseResult<Char> parse_char(std::string_view regex);
ParseResult<UnicodeCategoryName> parse_unicode_category_name(std::string_view regex);
ParseResult<CharacterClassFromUnicodeCategory> parse_character_class_from_unicode_category(std::string_view regex);
ParseResult<CharacterClass> parse_character_class(std::string_view regex);
ParseResult<CharacterRange> parse_character_range(std::string_view regex);
ParseResult<CharacterGroupItem> parse_character_group_item(std::string_view regex);
ParseResult<CharacterGroup> parse_character_group(std::string_view regex);
ParseResult<MatchCharacter> parse_match_character(std::string_view regex);
ParseResult<MatchCharacterClass> parse_match_character_class(std::string_view regex);
ParseResult<MatchItem> parse_match_item(std::string_view regex);
ParseResult<Match> parse_match(std::string_view regex);
ParseResult<Anchor> parse_anchor(std::string_view regex);
ParseResult<Backreference> parse_backreference(std::string_view regex);
ParseResult<Group> parse_group(std::string_view regex);
ParseResult<SubExpressionItem> parse_sub_expression_item(std::string_view regex);
ParseResult<SubExpression> parse_sub_expression(std::string_view regex);
ParseResult<Expression> parse_expression(std::string_view regex);
ParseResult<Regex> parse_regex(std::string_view regex);
}   // namespace kregex

#endif  // K_REGIX_ENGINE_PARSER_HPP