#ifndef K_REGIX_ENGINE_NFA_HPP
#define K_REGIX_ENGINE_NFA_HPP

#include <variant>
#include <vector>
#include <functional>
#include "parser_types.hpp"

namespace kregex
{
using StateId = std::size_t;
using GroupId = std::size_t;

struct Epsilon{};

struct GroupCaptureStart
{
    std::size_t group_number;

    inline bool operator==(const GroupCaptureStart& other) const
    {
        return group_number == other.group_number;
    }
    inline bool operator!=(const GroupCaptureStart& other) const
    {
        return !(*this == other);
    }
};

struct GroupCaptureEnd
{
    std::size_t group_number;

    inline bool operator==(const GroupCaptureEnd& other) const
    {
        return group_number == other.group_number;
    }
    inline bool operator!=(const GroupCaptureEnd& other) const
    {
        return !(*this == other);
    }
};

using TransitionConditionType = std::variant<
    Char,
    CharacterClass,
    CharacterRange,
    UnicodeCategoryName,
    CharacterGroup,
    MatchAnyCharacter,
    Anchor,
    StartOfStringAnchor,
    GroupCaptureStart,
    GroupCaptureEnd,
    Backreference,
    Epsilon>;

struct Transition
{
    StateId to_state;
    TransitionConditionType condition;
};

struct State
{
    StateId id;
    std::vector<Transition> transitions;
};

struct NfaElement
{
    StateId start;
    StateId accept;
};

class Nfa
{
    StateId m_current_id = 0;
    GroupId m_current_group_id = 1;
    std::vector<State> m_states;

    StateId add_new_state();
    void add_transition(StateId from, StateId to, const TransitionConditionType& condition);

    template<typename T>
    NfaElement build_basic_element(const T& value)
    {
        StateId start = add_new_state();
        StateId accept = add_new_state();
        add_transition(start, accept, value);
        return NfaElement{start, accept};
    }

    using NfaElementBuilder = std::function<NfaElement()>;

    NfaElement concatenate(const NfaElement& first, const NfaElement& second);
    NfaElement apply_range_quantifier(const RangeQuantifier& range_quantifier, std::optional<LazyModifier> lazy_modifier, NfaElementBuilder build_one_element);
    NfaElement apply_quantifier(const NfaElement& element, const Quantifier& quantifier, NfaElementBuilder build_one_element);
    NfaElement build_char(const Char& character);
    NfaElement build_unicode_category_name(const UnicodeCategoryName& name);
    NfaElement build_character_class_from_unicode_category(const CharacterClassFromUnicodeCategory& character_class_from_unicode_category);
    NfaElement build_character_class(const CharacterClass& character_class);
    NfaElement build_character_range(const CharacterRange& character_range);
    NfaElement build_character_group(const CharacterGroup& character_group);
    NfaElement build_match_character_class(const MatchCharacterClass& character_class);
    NfaElement build_match_item(const MatchItem& match_item);
    NfaElement build_match(const Match& match);
    NfaElement build_group_body(const Group& group, const GroupId& groupId);
    NfaElement build_group(const Group& group);
    NfaElement build_anchor(const Anchor& anchor);
    NfaElement build_backreference(const Backreference& backreference);
    NfaElement build_sub_expression_item(const SubExpressionItem& sub_expression_item);
    NfaElement build_sub_expression(const SubExpression& sub_expression);
    NfaElement build_expression(const Expression& expression);
public:
    Nfa() = default;
    ~Nfa() = default;

    NfaElement build_regex(const Regex& regex);
    const std::vector<State>& get_states() const;
};
}   // namespace kregex

#endif  // K_REGIX_ENGINE_NFA_HPP