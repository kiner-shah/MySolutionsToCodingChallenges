#ifndef K_REGIX_ENGINE_PARSER_TYPES_HPP
#define K_REGIX_ENGINE_PARSER_TYPES_HPP

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace kregex
{
using Integer = std::size_t;

using RangeQuantifierLowerBound = Integer;
using RangeQuantifierUpperBound = Integer;
using Backreference = Integer;

template<typename T>
using NodeRef = std::shared_ptr<T>;

struct RangeQuantifier
{
    bool exactly_lower_bound_times = false;
    RangeQuantifierLowerBound lower_bound;
    std::optional<RangeQuantifierUpperBound> upper_bound = std::nullopt;

    inline bool operator==(const RangeQuantifier& other) const
    {
        return exactly_lower_bound_times == other.exactly_lower_bound_times &&
               lower_bound == other.lower_bound &&
               upper_bound == other.upper_bound;
    }
    inline bool operator!=(const RangeQuantifier& other) const
    {
        return !(*this == other);
    }
};

enum class QuantifierType
{
    ZeroOrMore,
    OneOrMore,
    ZeroOrOne,
    Range
};

struct LazyModifier{};

struct Quantifier
{
    QuantifierType type;
    std::optional<RangeQuantifier> range_quantifier = std::nullopt;
    std::optional<LazyModifier> lazy_modifier = std::nullopt;

    inline bool operator==(const Quantifier& other) const
    {
        return type == other.type &&
               range_quantifier == other.range_quantifier &&
               lazy_modifier.has_value() == other.lazy_modifier.has_value();
    }
    inline bool operator!=(const Quantifier& other) const
    {
        return !(*this == other);
    }
};

struct CharacterGroupNegativeModifier{};

enum class CharacterClassType
{
    AnyWord,
    AnyWordInverted,
    AnyDecimalDigit,
    AnyDecimalDigitInverted,
    AnyWhitespace,
    AnyWhitespaceInverted
};

struct CharacterClass
{
    CharacterClassType type;

    inline bool operator==(const CharacterClass& other) const
    {
        return type == other.type;
    }
    inline bool operator!=(const CharacterClass& other) const
    {
        return !(*this == other);
    }
};

using Letters = std::string_view;
using Char = char32_t;

struct UnicodeCategoryName
{
    Letters letters;

    inline bool operator==(const UnicodeCategoryName& other) const
    {
        return letters == other.letters;
    }
    inline bool operator!=(const UnicodeCategoryName& other) const
    {
        return !(*this == other);
    }
};

struct CharacterClassFromUnicodeCategory
{
    UnicodeCategoryName category_name;

    inline bool operator==(const CharacterClassFromUnicodeCategory& other) const
    {
        return category_name == other.category_name;
    }
    inline bool operator!=(const CharacterClassFromUnicodeCategory& other) const
    {
        return !(*this == other);
    }
};

struct CharacterRange
{
    Char start;
    std::optional<Char> end = std::nullopt;
     
    inline bool operator==(const CharacterRange& other) const
    {
        return start == other.start && end == other.end;
    }
    inline bool operator!=(const CharacterRange& other) const
    {
        return !(*this == other);
    }
};

struct CharacterGroupItem
{
    std::variant<CharacterClass, CharacterClassFromUnicodeCategory, CharacterRange, Char> character_group_item;;

    inline bool operator==(const CharacterGroupItem& other) const
    {
        return character_group_item == other.character_group_item;
    }
    inline bool operator!=(const CharacterGroupItem& other) const
    {
        return !(*this == other);
    }
};

struct CharacterGroup
{
    std::optional<CharacterGroupNegativeModifier> negative_modifier = std::nullopt;
    std::vector<CharacterGroupItem> character_group_items;

    inline bool operator==(const CharacterGroup& other) const
    {
        return negative_modifier.has_value() == other.negative_modifier.has_value() &&
               character_group_items == other.character_group_items;
    }
    inline bool operator!=(const CharacterGroup& other) const
    {
        return !(*this == other);
    }
};


struct MatchCharacterClass
{
    std::variant<CharacterGroup, CharacterClass, CharacterClassFromUnicodeCategory> match_character_class;

    inline bool operator==(const MatchCharacterClass& other) const
    {
        return match_character_class == other.match_character_class;
    }
    inline bool operator!=(const MatchCharacterClass& other) const
    {
        return !(*this == other);
    }
};

struct MatchCharacter
{
    Char match_character;

    inline bool operator==(const MatchCharacter& other) const
    {
        return match_character == other.match_character;
    }
    inline bool operator!=(const MatchCharacter& other) const
    {
        return !(*this == other);
    }
};

struct MatchAnyCharacter
{
    inline bool operator==(const MatchAnyCharacter& other) const
    {
        return true;
    }
    inline bool operator!=(const MatchAnyCharacter& other) const
    {
        return false;
    }
};

struct MatchItem
{
    std::variant<MatchAnyCharacter, MatchCharacterClass, MatchCharacter> match_item;

    bool operator==(const MatchItem& other) const
    {
        return match_item == other.match_item;
    }
    bool operator!=(const MatchItem& other) const
    {
        return !(*this == other);
    }
};

struct Match
{
    MatchItem match_item;
    std::optional<Quantifier> quantifier = std::nullopt;

    bool operator==(const Match& other) const
    {
        return match_item == other.match_item && quantifier == other.quantifier;
    }
    bool operator!=(const Match& other) const
    {
        return !(*this == other);
    }
};

struct GroupNonCapturingModifier {};

struct Expression;
struct Group
{
    std::optional<GroupNonCapturingModifier> non_capturing_modifier = std::nullopt;
    NodeRef<Expression> expression;
    std::optional<Quantifier> quantifier = std::nullopt;

    inline bool operator==(const Group& other) const;
    inline bool operator!=(const Group& other) const;
};


enum class AnchorType
{
    WordBoundary,
    NonWordBoundary,
    StartOfStringOnly,
    EndOfStringOnlyNotNewline,
    EndOfStringOnly,
    PreviousMatchEnd,
    EndOfString
};

struct Anchor
{
    AnchorType type;
    
    inline bool operator==(const Anchor& other) const
    {
        return type == other.type;
    }
    inline bool operator!=(const Anchor& other) const
    {
        return !(*this == other);
    }
};

struct SubExpressionItem
{
    std::variant<Match, Group, Anchor, Backreference> item;

    inline bool operator==(const SubExpressionItem& other) const
    {
        return item == other.item;
    }
    inline bool operator!=(const SubExpressionItem& other) const
    {
        return !(*this == other);
    }
};

struct SubExpression
{
    std::vector<SubExpressionItem> sub_expression_items;

    inline bool operator==(const SubExpression& other) const
    {
        return sub_expression_items == other.sub_expression_items;
    }
    inline bool operator!=(const SubExpression& other) const
    {
        return !(*this == other);
    }
};

struct Expression
{
    std::vector<SubExpression> alternatives;

    inline bool operator==(const Expression& other) const
    {
        return alternatives == other.alternatives;
    }
    inline bool operator!=(const Expression& other) const
    {
        return !(*this == other);
    }
};

inline bool Group::operator==(const Group& other) const
{
    return non_capturing_modifier.has_value() == other.non_capturing_modifier.has_value()
        && (expression && other.expression && *expression == *other.expression)
        && quantifier == other.quantifier;
}
inline bool Group::operator!=(const Group& other) const
{
    return !(*this == other);
}

struct StartOfStringAnchor {};

struct Regex
{
    std::optional<StartOfStringAnchor> start_of_string_anchor = std::nullopt;
    Expression expression;

    inline bool operator==(const Regex& other) const
    {
        return start_of_string_anchor.has_value() == other.start_of_string_anchor.has_value()
            && expression == other.expression;
    }
    inline bool operator!=(const Regex& other) const
    {
        return !(*this == other);
    }
};
}   // namespace kregex

#endif  // K_REGIX_ENGINE_PARSER_TYPES_HPP