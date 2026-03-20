#include "nfa.hpp"
#include <string>

namespace kregex
{
StateId Nfa::add_new_state()
{
    std::size_t new_id = m_current_id;
    m_states.push_back(State{new_id, {}});
    ++m_current_id;
    return new_id;
}

void Nfa::add_transition(StateId from, StateId to, const TransitionConditionType& condition)
{
    m_states[from].transitions.emplace_back(Transition{to, condition});
}

NfaElement Nfa::concatenate(const NfaElement &first, const NfaElement &second)
{
    add_transition(first.accept, second.start, Epsilon{});
    return NfaElement{first.start, second.accept};
}

NfaElement Nfa::apply_range_quantifier(const RangeQuantifier& range_quantifier, std::optional<LazyModifier> lazy_modifier, NfaElementBuilder build_one_element)
{
    auto add_upper_bound = [this, &build_one_element, &lazy_modifier](const NfaElement& prev, std::size_t upper_bound) -> NfaElement
    {
        NfaElement result = prev;
        for (std::size_t i = 0; i < upper_bound; i++)
        {
            NfaElement next = build_one_element();
            NfaElement next_after_quantifier = apply_quantifier(next, Quantifier{QuantifierType::ZeroOrOne, std::nullopt, lazy_modifier}, build_one_element);
            result = concatenate(result, next_after_quantifier);
        }
        return result;
    };

    if (range_quantifier.lower_bound == 0)
    {
        if (range_quantifier.upper_bound.has_value() && range_quantifier.upper_bound.value() == 0)
        {
            // Handle cases - {0,0}
            return build_basic_element(Epsilon{});
        }
        else if (range_quantifier.exactly_lower_bound_times)
        {
            // Handle case - {0} (which is same as {0,0})
            return build_basic_element(Epsilon{});
        }
        else if (!range_quantifier.upper_bound.has_value() && !range_quantifier.exactly_lower_bound_times)
        {
            // Handle case - {0,}
            NfaElement zero_or_more_element = build_one_element();
            return apply_quantifier(zero_or_more_element, Quantifier{QuantifierType::ZeroOrMore, std::nullopt, lazy_modifier}, build_one_element);
        }
        else
        {
            // Handle case - {0,N} where N > 0
            NfaElement element = build_basic_element(Epsilon{});
            return add_upper_bound(element, range_quantifier.upper_bound.value());
        }
    }

    // Create N lower bound copies of the match and concatenate them
    NfaElement prev = build_one_element();
    for (std::size_t i = 0; i < range_quantifier.lower_bound - 1; ++i)
    {
        NfaElement next = build_one_element();
        next = concatenate(prev, next);
        prev = next;
    }
    // Then check if upper bound is present
    // If not, check for exactly_lower_bound_times, and process it accordingly
    if (!range_quantifier.upper_bound.has_value())
    {
        if (range_quantifier.exactly_lower_bound_times)
        {
            // Handle case - {N} where N > 0
            // prev contains the concatenated NfaElements
            // for the lower bound, so we can return it as is.
            return prev;
        }
        else
        {
            // Handle case - {N,} where N > 0
            NfaElement element = build_one_element();
            NfaElement tail_element = apply_quantifier(element, Quantifier{QuantifierType::ZeroOrMore, std::nullopt, lazy_modifier}, build_one_element);
            return concatenate(prev, tail_element);
        }
    }
    // If yes, process upper bound
    else
    {
        auto remaining_count = range_quantifier.upper_bound.value() - range_quantifier.lower_bound;
        return add_upper_bound(prev, remaining_count);
    }
    return NfaElement{};
}

NfaElement Nfa::apply_quantifier(const NfaElement &element, const Quantifier &quantifier, NfaElementBuilder build_one_element)
{
    switch (quantifier.type)
    {
    case QuantifierType::ZeroOrMore:
    {
        StateId loop_start = add_new_state();
        StateId loop_end = add_new_state();
        if (quantifier.lazy_modifier.has_value())
        {
            add_transition(loop_start, loop_end, Epsilon{});
            add_transition(loop_start, element.start, Epsilon{});
            add_transition(element.accept, loop_end, Epsilon{});
            add_transition(element.accept, element.start, Epsilon{});
        }
        else
        {
            add_transition(loop_start, element.start, Epsilon{});
            add_transition(loop_start, loop_end, Epsilon{});
            add_transition(element.accept, element.start, Epsilon{});
            add_transition(element.accept, loop_end, Epsilon{});
        }
        return NfaElement{loop_start, loop_end};
    }
    case QuantifierType::OneOrMore:
    {
        StateId loop_end = add_new_state();

        if (quantifier.lazy_modifier.has_value())
        {
            add_transition(element.accept, loop_end, Epsilon{});
            add_transition(element.accept, element.start, Epsilon{});
        }
        else
        {
            add_transition(element.accept, element.start, Epsilon{});
            add_transition(element.accept, loop_end, Epsilon{});
        }
        return NfaElement{element.start, loop_end};
    }
    case QuantifierType::ZeroOrOne:
    {
        StateId start = add_new_state();
        StateId accept = add_new_state();

        if (quantifier.lazy_modifier.has_value())
        {
            add_transition(start, accept, Epsilon{});
            add_transition(start, element.start, Epsilon{});
        }
        else
        {
            add_transition(start, element.start, Epsilon{});
            add_transition(start, accept, Epsilon{});
        }
        add_transition(element.accept, accept, Epsilon{});
        return NfaElement{start, accept};
    }
    case QuantifierType::Range:
    {
        if (!quantifier.range_quantifier.has_value())
        {
            throw std::invalid_argument("Range quantifier must have a range quantifier value");
        }
        const RangeQuantifier& range_quantifier = quantifier.range_quantifier.value();
        return apply_range_quantifier(range_quantifier, quantifier.lazy_modifier, build_one_element);
    }
    default:
        break;
    }
    return NfaElement{};
}

NfaElement Nfa::build_char(const Char &character)
{
    return build_basic_element(character);
}

NfaElement Nfa::build_unicode_category_name(const UnicodeCategoryName& name)
{
    return build_basic_element(name);
}

NfaElement Nfa::build_character_class_from_unicode_category(const CharacterClassFromUnicodeCategory& character_class_from_unicode_category)
{
    return build_unicode_category_name(character_class_from_unicode_category.category_name);
}

NfaElement Nfa::build_character_class(const CharacterClass& character_class)
{
    return build_basic_element(character_class);
}

NfaElement Nfa::build_character_range(const CharacterRange& character_range)
{
    return build_basic_element(character_range);
}

NfaElement Nfa::build_character_group(const CharacterGroup& character_group)
{
    return build_basic_element(character_group);
}

NfaElement Nfa::build_match_character_class(const MatchCharacterClass& character_class)
{
    struct Visitor
    {
        std::reference_wrapper<Nfa> nfa;

        NfaElement operator()(const CharacterGroup& character_group)
        {
            return nfa.get().build_character_group(character_group);
        }
        NfaElement operator()(const CharacterClass& character_class)
        {
            return nfa.get().build_character_class(character_class);
        }
        NfaElement operator()(const CharacterClassFromUnicodeCategory& character_class_from_unicode_category)
        {
            return nfa.get().build_character_class_from_unicode_category(character_class_from_unicode_category);
        }
    };

    Visitor visitor{*this};
    return std::visit(visitor, character_class.match_character_class);
}

NfaElement Nfa::build_match_item(const MatchItem& item)
{
    struct Visitor
    {
        std::reference_wrapper<Nfa> nfa;

        NfaElement operator()(const MatchCharacter& character)
        {
            return nfa.get().build_char(character.match_character);
        }
        NfaElement operator()(const MatchCharacterClass& character_class)
        {
            return nfa.get().build_match_character_class(character_class);
        }
        NfaElement operator()(const MatchAnyCharacter& any_character)
        {
            return nfa.get().build_basic_element(any_character);
        }
    };

    Visitor visitor{*this};
    return std::visit(visitor, item.match_item);
}

NfaElement Nfa::build_match(const Match& match)
{
    // NfaElement element = build_match_item(match.match_item);
    if (!match.quantifier.has_value())
    {
        return build_match_item(match.match_item);
    }
    else if (match.quantifier.value().type == QuantifierType::Range)
    {
        NfaElement dummy{};
        return apply_quantifier(dummy, match.quantifier.value(),
            [this, &match]() -> NfaElement
            {
                return build_match_item(match.match_item);
            });
    }
    else
    {
        NfaElement element = build_match_item(match.match_item);
        return apply_quantifier(element, match.quantifier.value(),
            [this, &match]() -> NfaElement
            {
                return build_match_item(match.match_item);
            });
    }
    return NfaElement{};
}

NfaElement Nfa::build_group_body(const Group& group, const GroupId& groupId)
{
    if (group.non_capturing_modifier.has_value())
    {
        if (group.expression)
        {
            return build_expression(*group.expression);
        }
        else
        {
            return build_basic_element(Epsilon{});
        }
    }
    else
    {
        StateId group_capture_start = add_new_state();
        StateId group_capture_end = add_new_state();

        if (group.expression)
        {
            NfaElement expression_element = build_expression(*group.expression);
            add_transition(group_capture_start, expression_element.start, GroupCaptureStart{groupId});
            add_transition(expression_element.accept, group_capture_end, GroupCaptureEnd{groupId});
            return NfaElement{group_capture_start, group_capture_end};
        }
        else
        {
            NfaElement dummy = build_basic_element(Epsilon{});
            add_transition(group_capture_start, dummy.start, GroupCaptureStart{groupId});
            add_transition(dummy.accept, group_capture_end, GroupCaptureEnd{groupId});
            return NfaElement{group_capture_start, group_capture_end};
        }
    }
    return NfaElement{};
}

NfaElement Nfa::build_group(const Group& group)
{
    GroupId current_group_id = m_current_group_id;
    if (!group.non_capturing_modifier.has_value())
    {
        m_current_group_id++;
    }
    if (!group.quantifier.has_value())
    {
        return build_group_body(group, current_group_id);
    }
    else if (group.quantifier.value().type == QuantifierType::Range)
    {
        NfaElement dummy{};
        return apply_quantifier(dummy, group.quantifier.value(),
            [this, &group, current_group_id]() -> NfaElement
            {
                return build_group_body(group, current_group_id);
            });
    }
    else
    {
        NfaElement group_body_element = build_group_body(group, current_group_id);
        return apply_quantifier(group_body_element, group.quantifier.value(),
            [this, &group, current_group_id]() -> NfaElement
            {
                return build_group_body(group, current_group_id);
            });
    }
    return NfaElement{};
}

NfaElement Nfa::build_anchor(const Anchor &anchor)
{
    return build_basic_element(anchor);
}

NfaElement Nfa::build_backreference(const Backreference &backreference)
{
    return build_basic_element(backreference);
}

NfaElement Nfa::build_sub_expression_item(const SubExpressionItem &sub_expression_item)
{
    struct Visitor
    {
        std::reference_wrapper<Nfa> nfa;

        NfaElement operator()(const Match& match)
        {
            return nfa.get().build_match(match);
        }
        NfaElement operator()(const Group& group)
        {
            return nfa.get().build_group(group);
        }
        NfaElement operator()(const Anchor& anchor)
        {
            return nfa.get().build_anchor(anchor);
        }
        NfaElement operator()(const Backreference& backreference)
        {
            return nfa.get().build_backreference(backreference);
        }
    };
    Visitor visitor{*this};
    return std::visit(visitor, sub_expression_item.item);
}

NfaElement Nfa::build_sub_expression(const SubExpression &sub_expression)
{
    if (sub_expression.sub_expression_items.empty())
    {
        return build_basic_element(Epsilon{});
    }

    NfaElement first = build_sub_expression_item(sub_expression.sub_expression_items.front());
    for (std::size_t i = 1; i < sub_expression.sub_expression_items.size(); ++i)
    {
        NfaElement next = build_sub_expression_item(sub_expression.sub_expression_items[i]);
        first = concatenate(first, next);
    }
    NfaElement final_element = first;
    return final_element;
}

NfaElement Nfa::build_expression(const Expression &expression)
{
    StateId start = add_new_state();
    StateId accept = add_new_state();

    for (const auto& alternative : expression.alternatives)
    {
        NfaElement element = build_sub_expression(alternative);
        add_transition(start, element.start, Epsilon{});
        add_transition(element.accept, accept, Epsilon{});
    }

    return NfaElement{start, accept};
}

NfaElement Nfa::build_regex(const Regex &regex)
{
    // TODO: implement this
    std::optional<NfaElement> start_of_string_anchor_element = std::nullopt;
    if (regex.start_of_string_anchor.has_value())
    {
        start_of_string_anchor_element = build_basic_element(regex.start_of_string_anchor.value());
    }
    NfaElement expression_element = build_expression(regex.expression);
    if (start_of_string_anchor_element.has_value())
    {
        return concatenate(start_of_string_anchor_element.value(), expression_element);
    }
    else
    {
        return expression_element;
    }
}

const std::vector<State> &Nfa::get_states() const
{
    return m_states;
}

} // namespace kregex