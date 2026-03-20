#include "parser.hpp"
#include <charconv>

namespace kregex
{
ParseResult<Integer> parse_number(std::string_view regex)
{
    std::size_t number = 0;
    auto result = std::from_chars(regex.data(), regex.data() + regex.size(), number);
    if (result.ec != std::errc{})
    {
        return ParseError{"Failed to parse number"};
    }
    return ParseValue<Integer>{number, std::string_view{result.ptr, static_cast<std::size_t>(regex.data() + regex.size() - result.ptr)}};
}

ParseResult<Letters> parse_letters(std::string_view regex)
{
    std::size_t index = regex.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
    if (index == 0)
    {
        return ParseError{"Expected at least one letter"};
    }
    std::string_view remaining_regex{};
    if (index != std::string_view::npos)
    {
        remaining_regex = regex.substr(index);
    }
    return ParseValue<Letters>{regex.substr(0, index), remaining_regex};
}

ParseResult<Quantifier> parse_quantifier(std::string_view regex)
{
    if (regex.empty())
    {
        return ParseError{"Empty quantifier"};
    }
    QuantifierType type{};
    switch (regex.front())
    {
        case '*':
            type = QuantifierType::ZeroOrMore;
            regex.remove_prefix(1);
            break;
        case '+':
            type = QuantifierType::OneOrMore;
            regex.remove_prefix(1);
            break;
        case '?':
            type = QuantifierType::ZeroOrOne;
            regex.remove_prefix(1);
            break;
        case '{':
            type = QuantifierType::Range;
            break;
        default:
            return ParseError{"Invalid quantifier"};
    }
    std::optional<RangeQuantifier> range_quantifier{};
    if (type == QuantifierType::Range)
    {
        auto result = parse_range_quantifier(regex);
        if (std::holds_alternative<ParseError>(result))
        {
            return std::get<ParseError>(result);
        }
        auto& [rq, remaining_regex] = std::get<ParseValue<RangeQuantifier>>(result);
        range_quantifier = rq;
        regex = remaining_regex;
    }
    std::optional<LazyModifier> lazy_modifier = std::nullopt;
    if (!regex.empty() && regex.front() == '?')
    {
        lazy_modifier = LazyModifier{};
        regex.remove_prefix(1);
    }
    return ParseValue<Quantifier>{Quantifier{type, range_quantifier, lazy_modifier}, regex};
}

ParseResult<RangeQuantifier> parse_range_quantifier(std::string_view regex)
{
    if (regex.empty() || regex.front() != '{')
    {
        return ParseError{"Expected '{' at the beginning of range quantifier"};
    }
    regex.remove_prefix(1);
    auto lb_result = parse_number(regex);
    if (std::holds_alternative<ParseError>(lb_result))
    {
        return std::get<ParseError>(lb_result);
    }

    RangeQuantifier range_quantifier;
    auto& [lower_bound, remaining_regex] = std::get<ParseValue<Integer>>(lb_result);
    if (remaining_regex.empty())
    {
        return ParseError{"Expected '}' at the end of range quantifier"};
    }
    range_quantifier.lower_bound = lower_bound;

    if (remaining_regex.front() == '}')
    {
        range_quantifier.exactly_lower_bound_times = true;
        return ParseValue<RangeQuantifier>{range_quantifier, remaining_regex.substr(1)};
    }
    else if (remaining_regex.front() == ',')
    {
        remaining_regex.remove_prefix(1);
        if (remaining_regex.empty())
        {
            return ParseError{"Expected '}' at the end of range quantifier"};
        }
        if (remaining_regex.front() == '}')
        {
            return ParseValue<RangeQuantifier>{range_quantifier, remaining_regex.substr(1)};
        }
        else
        {
            auto ub_result = parse_number(remaining_regex);
            if (std::holds_alternative<ParseError>(ub_result))
            {
                return std::get<ParseError>(ub_result);
            }
            auto& [upper_bound, remaining_regex_after_ub] = std::get<ParseValue<Integer>>(ub_result);
            range_quantifier.upper_bound = upper_bound;
            if (range_quantifier.lower_bound > range_quantifier.upper_bound.value())
            {
                return ParseError{"Lower bound cannot be greater than upper bound in range quantifier"};
            }
            if (remaining_regex_after_ub.empty() || remaining_regex_after_ub.front() != '}')
            {
                return ParseError{"Expected '}' at the end of range quantifier"};
            }
            return ParseValue<RangeQuantifier>{range_quantifier, remaining_regex_after_ub.substr(1)};
        }
    }
    return ParseError{"Expected ',' or '}' in range quantifier"};
}

ParseResult<Char> parse_char(std::string_view regex)
{
    if (regex.empty())
    {
        return ParseError{"Empty character"};
    }
    // UTF-8 decoding algorithm
    char32_t utf8_code_point = 0;
    std::size_t i = 0;
    unsigned char c = regex[i];
    if (c <= 0x7F)
    {
        utf8_code_point = c;
        ++i;
    }
    else if (c >= 0xC0 && c <= 0xDF)
    {
        if (i + 1 >= regex.size())
        {
            return ParseError{"Invalid UTF-8 byte sequence for codepoint range U+0080 - U+07FF"};
        }
        unsigned char c1 = regex[i + 1];
        if (c1 >= 0x80 && c1 <= 0xBF)
        {
            utf8_code_point = ((c & 0x1F) << 6) | (c1 & 0x3F);
            i += 2;
        }
        else
        {
            return ParseError{"Invalid UTF-8 byte sequence for codepoint range U+0080 - U+07FF"};
        }
    }
    else if (c >= 0xE0 && c <= 0xEF)
    {
        if (i + 1 >= regex.size() || i + 2 >= regex.size())
        {
            return ParseError{"Invalid UTF-8 byte sequence for codepoint range U+0800 - U+FFFF"};
        }
        unsigned char c1 = regex[i + 1];
        unsigned char c2 = regex[i + 2];
        if (c1 >= 0x80 && c1 <= 0xBF && c2 >= 0x80 && c2 <= 0xBF)
        {
            utf8_code_point = ((c & 0xF) << 12) | ((c1 & 0x3F) << 6) | (c2 & 0x3F);
            i += 3;
        }
        else
        {
            return ParseError{"Invalid UTF-8 byte sequence for codepoint range U+0080 - U+07FF"};
        }
    }
    else if (c >= 0xF0 && c <= 0xF7)
    {
        if (i + 1 >= regex.size() || i + 2 >= regex.size() || i + 3 >= regex.size())
        {
            return ParseError{"Invalid UTF-8 byte sequence for codepoint range U+10000 - U+10FFFF"};
        }
        unsigned char c1 = regex[i + 1];
        unsigned char c2 = regex[i + 2];
        unsigned char c3 = regex[i + 3];
        if (c1 >= 0x80 && c1 <= 0xBF && c2 >= 0x80 && c2 <= 0xBF && c3 >= 0x80 && c3 <= 0xBF)
        {
            utf8_code_point = ((c & 0x7) << 18) | ((c1 & 0x3F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
            i += 4;
        }
        else
        {
            return ParseError{"Invalid UTF-8 byte sequence for codepoint range U+10000 - U+10FFFF"};
        }
    }
    else
    {
        return ParseError{"Invalid UTF-8 byte sequence"};
    }
    
    return ParseValue<Char>{utf8_code_point, regex.substr(i)};
}

ParseResult<UnicodeCategoryName> parse_unicode_category_name(std::string_view regex)
{
    if (regex.empty())
    {
        return ParseError{"Empty unicode category name"};
    }
    auto result = parse_letters(regex);
    if (std::holds_alternative<ParseError>(result))
    {
        return std::get<ParseError>(result);
    }
    auto& [letters, remaining_regex] = std::get<ParseValue<Letters>>(result);
    return ParseValue<UnicodeCategoryName>{UnicodeCategoryName{letters}, remaining_regex};
}

ParseResult<CharacterClassFromUnicodeCategory> parse_character_class_from_unicode_category(std::string_view regex)
{
    if (regex.empty())
    {
        return ParseError{"Empty character class from unicode category"};
    }
    if (!regex.starts_with("\\p{"))
    {
        return ParseError{"Expected '\\p{' at the beginning of character class from unicode category"};
    }
    regex.remove_prefix(3);
    auto result = parse_unicode_category_name(regex);
    if (std::holds_alternative<ParseError>(result))
    {
        return std::get<ParseError>(result);
    }
    auto& [category_name, remaining_regex] = std::get<ParseValue<UnicodeCategoryName>>(result);
    if (remaining_regex.empty() || remaining_regex.front() != '}')
    {
        return ParseError{"Expected '}' at the end of character class from unicode category"};
    }
    remaining_regex.remove_prefix(1);
    return ParseValue<CharacterClassFromUnicodeCategory>{CharacterClassFromUnicodeCategory{category_name}, remaining_regex};
}

ParseResult<CharacterClass> parse_character_class(std::string_view regex)
{
    if (regex.empty())
    {
        return ParseError{"Empty character class"};
    }
    CharacterClassType type{};
    if (regex.front() != '\\')
    {
        return ParseError{"Expected '\\' at the beginning of character class"};
    }
    regex.remove_prefix(1);
    if (regex.empty())
    {
        return ParseError{"Expected a character class type after '\\'"};
    }
    switch (regex.front())
    {
    case 'w':
        type = CharacterClassType::AnyWord;
        break;
    case 'W':
        type = CharacterClassType::AnyWordInverted;
        break;
    case 'd':
        type = CharacterClassType::AnyDecimalDigit;
        break;
    case 'D':
        type = CharacterClassType::AnyDecimalDigitInverted;
        break;
    default:
        return ParseError{"Invalid character class type"};
    }
    return ParseValue<CharacterClass>{CharacterClass{type}, regex.substr(1)};
}

ParseResult<CharacterRange> parse_character_range(std::string_view regex)
{
    if (regex.empty())
    {
        return ParseError{"Empty character range"};
    }
    if (regex.front() == '-')
    {
        return ParseError{"Character range cannot start with '-'"};
    }
    auto result = parse_char(regex);
    if (std::holds_alternative<ParseError>(result))
    {
        return std::get<ParseError>(result);
    }
    auto& [start, remaining_regex] = std::get<ParseValue<Char>>(result);
    
    CharacterRange char_range;
    char_range.start = start;
    if (!remaining_regex.empty() && remaining_regex.front() == '-')
    {
        remaining_regex.remove_prefix(1);
        auto end_result = parse_char(remaining_regex);
        if (std::holds_alternative<ParseError>(end_result))
        {
            return std::get<ParseError>(end_result);
        }
        auto& [end, remaining_regex_after_end] = std::get<ParseValue<Char>>(end_result);
        if (start > end)
        {
            return ParseError{"Start character cannot be greater than end character in character range"};
        }
        char_range.end = end;
        return ParseValue<CharacterRange>{char_range, remaining_regex_after_end};
    }
    return ParseValue<CharacterRange>{char_range, remaining_regex};
}

ParseResult<CharacterGroupItem> parse_character_group_item(std::string_view regex)
{
    if (regex.empty())
    {
        return ParseError{"Empty character group item"};
    }
    auto character_class_result = parse_character_class(regex);
    if (std::holds_alternative<ParseError>(character_class_result))
    {
        auto character_class_from_unicode_category_result = parse_character_class_from_unicode_category(regex);
        if (std::holds_alternative<ParseError>(character_class_from_unicode_category_result))
        {
            auto character_range_result = parse_character_range(regex);
            if (std::holds_alternative<ParseError>(character_range_result))
            {
                auto char_result = parse_char(regex);
                if (std::holds_alternative<ParseError>(char_result))
                {
                    ParseError error{"Expected a character class or a character class from unicode category or a character range or a character at the beginning of character group item\n"};
                    error += "  Character class parse error: " + std::get<ParseError>(character_class_result);
                    error += "\n  Character class from unicode category parse error: " + std::get<ParseError>(character_class_from_unicode_category_result);
                    error += "\n  Character range parse error: " + std::get<ParseError>(character_range_result);
                    error += "\n  Character parse error: " + std::get<ParseError>(char_result);
                    return error;
                }
                auto& [match_character, remaining_regex_after_character] = std::get<ParseValue<Char>>(char_result);
                return ParseValue<CharacterGroupItem>{CharacterGroupItem{match_character}, remaining_regex_after_character};
            }
            auto& [character_range, remaining_regex_after_character_range] = std::get<ParseValue<CharacterRange>>(character_range_result);
            return ParseValue<CharacterGroupItem>{CharacterGroupItem{character_range}, remaining_regex_after_character_range};
        }
        auto& [character_class_from_unicode_category, remaining_regex_after_character_class_from_unicode_category] = std::get<ParseValue<CharacterClassFromUnicodeCategory>>(character_class_from_unicode_category_result);
        return ParseValue<CharacterGroupItem>{CharacterGroupItem{character_class_from_unicode_category}, remaining_regex_after_character_class_from_unicode_category};
    }
    auto& [character_class, remaining_regex_after_character_class] = std::get<ParseValue<CharacterClass>>(character_class_result);
    return ParseValue<CharacterGroupItem>{CharacterGroupItem{character_class}, remaining_regex_after_character_class};
}

ParseResult<CharacterGroup> parse_character_group(std::string_view regex)
{
    if (regex.empty())
    {
        return ParseError{"Empty character group"};
    }
    if (regex.front() != '[')
    {
        return ParseError{"Expected '[' at the beginning of character group"};
    }
    regex.remove_prefix(1);
    if (regex.empty())
    {
        return ParseError{"Expected ']' at the end of character group"};
    }

    CharacterGroup character_group;
    if (regex.front() == '^')
    {
        character_group.negative_modifier = CharacterGroupNegativeModifier{};
        regex.remove_prefix(1);
    }
    while (!regex.empty() && regex.front() != ']')
    {
        auto result = parse_character_group_item(regex);
        if (std::holds_alternative<ParseError>(result))
        {
            break;
        }
        auto& [character_group_item, remaining_regex] = std::get<ParseValue<CharacterGroupItem>>(result);
        character_group.character_group_items.push_back(character_group_item);
        regex = remaining_regex;
    }
    if (regex.empty() || regex.front() != ']')
    {
        return ParseError{"Expected ']' at the end of character group"};
    }
    regex.remove_prefix(1);
    return ParseValue<CharacterGroup>{character_group, regex};
}

ParseResult<MatchCharacter> parse_match_character(std::string_view regex)
{
    if (regex.empty())
    {
        return ParseError{"Empty match character"};
    }
    auto result = parse_char(regex);
    if (std::holds_alternative<ParseError>(result))
    {
        return std::get<ParseError>(result);
    }
    auto& [character, remaining_regex] = std::get<ParseValue<Char>>(result);
    return ParseValue<MatchCharacter>{MatchCharacter{character}, remaining_regex};
}

ParseResult<MatchCharacterClass> parse_match_character_class(std::string_view regex)
{
    if (regex.empty())
    {
        return ParseError{"Empty match character class"};
    }
    auto cg_result = parse_character_group(regex);
    if (std::holds_alternative<ParseError>(cg_result))
    {
        auto cc_result = parse_character_class(regex);
        if (std::holds_alternative<ParseError>(cc_result))
        {
            auto cc_from_uc_result = parse_character_class_from_unicode_category(regex);
            if (std::holds_alternative<ParseError>(cc_from_uc_result))
            {
                ParseError error{"Expected a character group or a character class or a character class from unicode category at the beginning of match character class\n"};
                error += "  Character group parse error: " + std::get<ParseError>(cg_result);
                error += "\n  Character class parse error: " + std::get<ParseError>(cc_result);
                error += "\n  Character class from unicode category parse error: " + std::get<ParseError>(cc_from_uc_result);
                return error;
            }
            auto& [character_class_from_unicode_category, remaining_regex_after_cc_from_uc] = std::get<ParseValue<CharacterClassFromUnicodeCategory>>(cc_from_uc_result);
            return ParseValue<MatchCharacterClass>{MatchCharacterClass{character_class_from_unicode_category}, remaining_regex_after_cc_from_uc};
        }
        auto& [character_class, remaining_regex_after_cc] = std::get<ParseValue<CharacterClass>>(cc_result);
        return ParseValue<MatchCharacterClass>{MatchCharacterClass{character_class}, remaining_regex_after_cc};
    }
    auto& [character_group, remaining_regex_after_cg] = std::get<ParseValue<CharacterGroup>>(cg_result);
    return ParseValue<MatchCharacterClass>{MatchCharacterClass{character_group}, remaining_regex_after_cg};
}

ParseResult<MatchItem> parse_match_item(std::string_view regex)
{
    if (regex.empty())
    {
        return ParseError{"Empty match item"};
    }
    if (regex.front() == '.')
    {
        return ParseValue<MatchItem>{MatchItem{MatchAnyCharacter{}}, regex.substr(1)};
    }
    auto match_character_class_result = parse_match_character_class(regex);
    if (std::holds_alternative<ParseError>(match_character_class_result))
    {
        auto match_character_result = parse_match_character(regex);
        if (std::holds_alternative<ParseError>(match_character_result))
        {
            ParseError error{"Expected a character class or a character at the beginning of match item\n"};
            error += "  Character class parse error: " + std::get<ParseError>(match_character_class_result);
            error += "\n  Character parse error: " + std::get<ParseError>(match_character_result);
            return error;
        }
        auto& [match_character, remaining_regex_after_character] = std::get<ParseValue<MatchCharacter>>(match_character_result);
        return ParseValue<MatchItem>{MatchItem{match_character}, remaining_regex_after_character};
    }
    auto& [character_class, remaining_regex] = std::get<ParseValue<MatchCharacterClass>>(match_character_class_result);
    return ParseValue<MatchItem>{MatchItem{character_class}, remaining_regex};
}

ParseResult<Match> parse_match(std::string_view regex)
{
    if (regex.empty())
    {
        return ParseError{"Empty match"};
    }
    auto result = parse_match_item(regex);
    if (std::holds_alternative<ParseError>(result))
    {
        return std::get<ParseError>(result);
    }
    auto& [match_item, remaining_regex] = std::get<ParseValue<MatchItem>>(result);
    Match match{match_item, std::nullopt};

    if (!remaining_regex.empty())
    {
        auto quantifier_result = parse_quantifier(remaining_regex);
        if (std::holds_alternative<ParseError>(quantifier_result))
        {
            return ParseValue<Match>{match, remaining_regex};
        }
        auto& [quantifier, remaining_regex_after_quantifier] = std::get<ParseValue<Quantifier>>(quantifier_result);
        match.quantifier = quantifier;
        return ParseValue<Match>{match, remaining_regex_after_quantifier};
    }
    return ParseValue<Match>{match, remaining_regex};
}

ParseResult<Anchor> parse_anchor(std::string_view regex)
{
    if (regex.empty())
    {
        return ParseError{"Empty anchor"};
    }

    if (regex.front() == '$')
    {
        return ParseValue<Anchor>{Anchor{AnchorType::EndOfString}, regex.substr(1)};
    }

    if (regex.front() != '\\')
    {
        return ParseError{"Expected '\\' at the beginning of anchor"};
    }
    regex.remove_prefix(1);

    AnchorType type{};
    if (!regex.empty())
    {
        switch (regex.front())
        {
        case 'b': type = AnchorType::WordBoundary; break;
        case 'B': type = AnchorType::NonWordBoundary; break;
        case 'A': type = AnchorType::StartOfStringOnly; break;
        case 'z': type = AnchorType::EndOfStringOnlyNotNewline; break;
        case 'Z': type = AnchorType::EndOfStringOnly; break;
        case 'G': type = AnchorType::PreviousMatchEnd; break;
        default:
            return ParseError{"Invalid anchor type"};
        }
    }
    else
    {
        return ParseError{"Expected an anchor type after '\\'"};
    }
    return ParseValue<Anchor>{Anchor{type}, regex.substr(1)};
}

ParseResult<Backreference> parse_backreference(std::string_view regex)
{
    if (regex.empty())
    {
        return ParseError{"Empty backreference"};
    }
    if (regex.front() != '\\')
    {
        return ParseError{"Expected '\\' at the beginning of backreference"};
    }
    regex.remove_prefix(1);
    auto result = parse_number(regex);
    if (std::holds_alternative<ParseError>(result))
    {
        return ParseError{"Failed to parse backreference number"};
    }
    auto& [number, remaining_regex] = std::get<ParseValue<Integer>>(result);
    return ParseValue<Backreference>{Backreference{number}, remaining_regex};
}

ParseResult<Group> parse_group(std::string_view regex)
{
    if (regex.empty())
    {
        return ParseError{"Empty group"};
    }
    if (regex.front() != '(')
    {
        return ParseError{"Expected '(' at the beginning of group"};
    }
    regex.remove_prefix(1);

    Group group;
    if (regex.starts_with("?:"))
    {
        group.non_capturing_modifier = GroupNonCapturingModifier{};
        regex.remove_prefix(2);
    }
    auto expression_result = parse_expression(regex);
    if (std::holds_alternative<ParseError>(expression_result))
    {
        return std::get<ParseError>(expression_result);
    }
    auto& [expression, remaining_regex] = std::get<ParseValue<Expression>>(expression_result);
    group.expression = std::make_shared<Expression>(expression);
    
    if (remaining_regex.empty() || remaining_regex.front() != ')')
    {
        return ParseError{"Expected ')' at the end of group"};
    }
    remaining_regex.remove_prefix(1);

    auto quantifier_result = parse_quantifier(remaining_regex);
    if (std::holds_alternative<ParseError>(quantifier_result))
    {
        return ParseValue<Group>{group, remaining_regex};
    }
    auto& [quantifier, remaining_regex_after_quantifier] = std::get<ParseValue<Quantifier>>(quantifier_result);
    group.quantifier = quantifier;
    return ParseValue<Group>{group, remaining_regex_after_quantifier};
}

ParseResult<SubExpressionItem> parse_sub_expression_item(std::string_view regex)
{
    if (regex.empty())
    {
        return ParseError{"Empty sub-expression item"};
    }
    if (regex.front() == '|' || regex.front() == ')')
    {
        return ParseError{"Sub-expression item cannot start with '|' or ')'"};
    }

    switch (regex.front())
    {
    case '(':
    {
        auto result = parse_group(regex);
        if (std::holds_alternative<ParseError>(result))
        {
            return std::get<ParseError>(result);
        }
        auto& [group, remaining_regex] = std::get<ParseValue<Group>>(result);
        return ParseValue<SubExpressionItem>{SubExpressionItem{group}, remaining_regex};
    }
    case '\\':
    {
        auto a_result = parse_anchor(regex);
        if (std::holds_alternative<ParseError>(a_result))
        {
            auto br_result = parse_backreference(regex);
            if (std::holds_alternative<ParseError>(br_result))
            {
                auto match_result = parse_match(regex);
                if (std::holds_alternative<ParseError>(match_result))
                {
                    ParseError error{"Expected an anchor or a backreference or a match at the beginning of sub-expression item\n"};
                    error += "  Anchor parse error: " + std::get<ParseError>(a_result);
                    error += "\n  Backreference parse error: " + std::get<ParseError>(br_result);
                    error += "\n  Match parse error: " + std::get<ParseError>(match_result);
                    return error;
                }
                auto& [match, remaining_regex_after_match] = std::get<ParseValue<Match>>(match_result);
                return ParseValue<SubExpressionItem>{SubExpressionItem{match}, remaining_regex_after_match};
            }
            auto& [backreference, remaining_regex_after_br] = std::get<ParseValue<Backreference>>(br_result);
            return ParseValue<SubExpressionItem>{SubExpressionItem{backreference}, remaining_regex_after_br};
        }
        auto& [anchor, remaining_regex_after_anchor] = std::get<ParseValue<Anchor>>(a_result);
        return ParseValue<SubExpressionItem>{SubExpressionItem{anchor}, remaining_regex_after_anchor};
    }
    case '$':
    {
        auto result = parse_anchor(regex);
        if (std::holds_alternative<ParseError>(result))
        {
            return std::get<ParseError>(result);
        }
        auto& [anchor, remaining_regex] = std::get<ParseValue<Anchor>>(result);
        return ParseValue<SubExpressionItem>{SubExpressionItem{anchor}, remaining_regex};
    }
    default:
    {
        auto result = parse_match(regex);
        if (std::holds_alternative<ParseError>(result))
        {
            return std::get<ParseError>(result);
        }
        auto& [match, remaining_regex] = std::get<ParseValue<Match>>(result);
        return ParseValue<SubExpressionItem>{SubExpressionItem{match}, remaining_regex};
    }
    }
    return ParseError{"Invalid sub-expression item"};
}

ParseResult<SubExpression> parse_sub_expression(std::string_view regex)
{
    if (regex.empty())
    {
        return ParseError{"Empty sub-expression"};
    }
    auto result = parse_sub_expression_item(regex);
    if (std::holds_alternative<ParseError>(result))
    {
        return std::get<ParseError>(result);
    }
    SubExpression sub_expression;
    auto& [parsed_item, remaining_regex] = std::get<ParseValue<SubExpressionItem>>(result);
    sub_expression.sub_expression_items.push_back(parsed_item);
    while (!remaining_regex.empty() && remaining_regex.front() != '|' && remaining_regex.front() != ')')
    {
        result = parse_sub_expression_item(remaining_regex);
        if (std::holds_alternative<ParseError>(result))
        {
            return std::get<ParseError>(result);
        }
        auto& [parsed_item, remaining_regex_temp] = std::get<ParseValue<SubExpressionItem>>(result);
        sub_expression.sub_expression_items.push_back(parsed_item);
        remaining_regex = remaining_regex_temp;
    }

    return ParseValue<SubExpression>{sub_expression, remaining_regex};
}

ParseResult<Expression> parse_expression(std::string_view regex)
{
    if (regex.empty())
    {
        return ParseError{"Empty expression"};
    }
    auto result = parse_sub_expression(regex);
    if (std::holds_alternative<ParseError>(result))
    {
        return std::get<ParseError>(result);
    }
    Expression expression;
    auto& [parsed_sub_expression, remaining_regex] = std::get<ParseValue<SubExpression>>(result);
    expression.alternatives.push_back(parsed_sub_expression);

    if (remaining_regex.empty())
    {
        return ParseValue<Expression>{expression, remaining_regex};
    }
    while (remaining_regex.front() == '|')
    {
        remaining_regex.remove_prefix(1);
        auto expression_result = parse_sub_expression(remaining_regex);
        if (std::holds_alternative<ParseError>(expression_result))
        {
            return std::get<ParseError>(expression_result);
        }
        auto& [parsed_sub_expression, remaining_regex_temp] = std::get<ParseValue<SubExpression>>(expression_result);
        expression.alternatives.push_back(parsed_sub_expression);
        remaining_regex = remaining_regex_temp;
        if (remaining_regex.empty())
        {
            break;
        }
    }

    return ParseValue<Expression>{expression, remaining_regex};
}

ParseResult<Regex> parse_regex(std::string_view regex)
{
    if (regex.empty())
    {
        return ParseError{"Empty regex"};
    }

    Regex parsed_regex;
    if (regex.front() == '^')
    {
        parsed_regex.start_of_string_anchor = StartOfStringAnchor{};
        regex.remove_prefix(1);
    }
    auto expression_result = parse_expression(regex);
    if (std::holds_alternative<ParseError>(expression_result))
    {
        return std::get<ParseError>(expression_result);
    }
    auto& [parsed_expression, remaining_regex] = std::get<ParseValue<Expression>>(expression_result);
    parsed_regex.expression = parsed_expression;
    return ParseValue<Regex>{parsed_regex, remaining_regex};
}
} // namespace kregex