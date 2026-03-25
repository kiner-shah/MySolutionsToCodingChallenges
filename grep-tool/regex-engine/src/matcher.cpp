#include "matcher.hpp"
#include <algorithm>
#include <queue>
#include <set>
#include <stdexcept>
#include <tuple>

namespace kregex
{
Matcher::Matcher(const NfaElement& regex_nfa_element, const std::vector<State>& states)
    : m_regex_nfa_element{regex_nfa_element}, m_states{states}
{
}

std::optional<TransitionResult> Matcher::process_transition(
    MatchState& match_state,
    StateId to_state_id,
    const TransitionConditionType& transition_condition,
    std::u32string_view input,
    bool negate)
{
    struct Visitor
    {
        std::reference_wrapper<Matcher> matcher;
        std::reference_wrapper<MatchState> match_state;
        std::u32string_view input;
        StateId to_state_id;
        bool negate;

        std::optional<TransitionResult> operator()(const Epsilon&) const
        {
            return TransitionResult{0, to_state_id};
        }
        std::optional<TransitionResult> operator()(const Char& transition_char) const
        {
            return matcher.get().process_char_transition(match_state.get(), to_state_id, transition_char, input, negate);
        }
        std::optional<TransitionResult> operator()(const CharacterClass& transition_character_class) const
        {
            return matcher.get().process_character_class_transition(match_state.get(), to_state_id, transition_character_class, input, negate);
        }
        std::optional<TransitionResult> operator()(const CharacterRange& transition_character_range) const
        {
            return matcher.get().process_character_range_transition(match_state.get(), to_state_id, transition_character_range, input, negate);
        }
        std::optional<TransitionResult> operator()(const UnicodeCategoryName& transition_unicode_category_name) const
        {
            return matcher.get().process_unicode_category_name_transition(match_state.get(), to_state_id, transition_unicode_category_name, input, negate);
        }
        std::optional<TransitionResult> operator()(const CharacterGroup& transition_character_group) const
        {
            return matcher.get().process_character_group_transition(match_state.get(), to_state_id, transition_character_group, input);
        }
        std::optional<TransitionResult> operator()(const MatchAnyCharacter& transition_match_any_character) const
        {
            return TransitionResult{1, to_state_id};
        }
        std::optional<TransitionResult> operator()(const Anchor& transition_anchor) const
        {
            return matcher.get().process_anchor_transition(match_state.get(), to_state_id, transition_anchor, input);
        }
        std::optional<TransitionResult> operator()(const StartOfStringAnchor& transition_start_of_string_anchor) const
        {
            return matcher.get().process_start_of_string_anchor_transition(match_state.get(), to_state_id, transition_start_of_string_anchor, input);
        }
        std::optional<TransitionResult> operator()(const GroupCaptureStart& transition_group_capture_start) const
        {
            return matcher.get().process_group_capture_start_transition(match_state.get(), to_state_id, transition_group_capture_start, input);
        }
        std::optional<TransitionResult> operator()(const GroupCaptureEnd& transition_group_capture_end) const
        {
            return matcher.get().process_group_capture_end_transition(match_state.get(), to_state_id, transition_group_capture_end, input);
        }
        std::optional<TransitionResult> operator()(const Backreference& transition_backreference) const
        {
            return matcher.get().process_backreference_transition(match_state.get(), to_state_id, transition_backreference, input);
        }
    };
    Visitor visitor{std::ref(*this), std::ref(match_state), input, to_state_id, negate};
    return std::visit(visitor, transition_condition);
}

std::optional<MatchResult> Matcher::match_one(std::u32string_view input)
{
    MatchResult result{};
    MatchState current_match_state{input, 0, m_regex_nfa_element.start, {}};
    std::queue<MatchState> states_to_process;
    std::set<std::tuple<StateId, std::size_t, std::vector<Range>>> visited_states;  // tuple of state id, input position, and group ranges

    // Run breadth-first search path finding
    states_to_process.push(current_match_state);
    while (!states_to_process.empty())
    {
        MatchState current_match_state = states_to_process.front();
        states_to_process.pop();

        if (visited_states.count({
            current_match_state.current_state_id,
            current_match_state.input_position,
            current_match_state.group_ranges ? *current_match_state.group_ranges : std::vector<Range>{}
        }) > 0)
        {
            continue;
        }
        visited_states.insert({
            current_match_state.current_state_id,
            current_match_state.input_position,
            current_match_state.group_ranges ? *current_match_state.group_ranges : std::vector<Range>{}
        });

        if (current_match_state.current_state_id == m_regex_nfa_element.accept)
        {
            if (current_match_state.group_ranges)
            {
                // Remove any group range with incomplete start or end position
                current_match_state.group_ranges->erase(
                    std::remove_if(
                        current_match_state.group_ranges->begin(),
                        current_match_state.group_ranges->end(),
                        [](const Range& group_range)
                        {
                            return !group_range.from_position.has_value() || !group_range.to_position.has_value();
                        }),
                    current_match_state.group_ranges->end());
            }
            result.group_ranges = current_match_state.group_ranges ? *current_match_state.group_ranges : std::vector<Range>{};
            // Insert whole match range at beginning of list of group ranges
            result.group_ranges.insert(result.group_ranges.begin(), Range{0, 0, current_match_state.input_position});
            return result;
        }

        const auto& transitions = m_states.at(current_match_state.current_state_id).transitions;
        for (const auto& transition : transitions)
        {
            MatchState branch_state = current_match_state;
            if (current_match_state.group_ranges)
            {
                branch_state.group_ranges = std::make_shared<std::vector<Range>>(*current_match_state.group_ranges);
            }
            auto transition_result = process_transition(
                branch_state,
                transition.to_state,
                transition.condition,
                input.substr(branch_state.input_position));
            if (transition_result.has_value())
            {
                auto [offset, to_state_id] = transition_result.value();
                branch_state.input_position += offset;
                branch_state.current_state_id = to_state_id;
                states_to_process.push(branch_state);
            }
        }
    }
    return std::nullopt;
}

std::vector<MatchResult> Matcher::match_all(std::u32string_view input)
{
    std::vector<MatchResult> results{};
    std::size_t i = 0;
    do
    {
        auto result = match_one(input.substr(i));
        if (!result.has_value())
        {
            i++;
            continue;
        }
        results.push_back(result.value());
        const auto& whole_match_range = result->group_ranges.at(0);
        // to_position and from_position should always have value for whole match range
        std::size_t consumed = whole_match_range.to_position.value() - whole_match_range.from_position.value();
        i += (consumed > 0 ? consumed : 1);
    } while (i < input.size());
    return results;
}

std::optional<TransitionResult> Matcher::process_char_transition(MatchState& match_state, StateId to_state_id, const Char &character, std::u32string_view input, bool negate)
{
    if (input.empty())
    {
        return std::nullopt;
    }
    bool condition = input.at(0) == character;
    if (negate)
    {
        condition = !condition;
    }
    return condition ? std::make_optional<TransitionResult>(TransitionResult{1, to_state_id}) : std::nullopt;
}

std::optional<TransitionResult> Matcher::process_character_class_transition(MatchState &match_state, StateId to_state_id, const CharacterClass &character_class, std::u32string_view input, bool negate)
{
    if (input.empty())
    {
        return std::nullopt;
    }

    switch (character_class.type)
    {
    case CharacterClassType::AnyDecimalDigit:
    {
        auto c = input.at(0);
        bool condition = (c >= U'0' && c <= U'9');
        if (negate)
        {
            condition = !condition;
        }
        return condition ? std::make_optional<TransitionResult>(TransitionResult{1, to_state_id}) : std::nullopt;
    }
    case CharacterClassType::AnyDecimalDigitInverted:
    {
        auto c = input.at(0);
        bool condition = (c < U'0' || c > U'9');
        if (negate)
        {
            condition = !condition;
        }
        return condition ? std::make_optional<TransitionResult>(TransitionResult{1, to_state_id}) : std::nullopt;
    }
    case CharacterClassType::AnyWord:
    {
        auto c = input.at(0);
        bool condition = ((c >= U'a' && c <= U'z')
                        || (c >= U'A' && c <= U'Z')
                        || (c >= U'0' && c <= U'9')
                        || c == U'_');
        if (negate)
        {
            condition = !condition;
        }
        return condition ? std::make_optional<TransitionResult>(TransitionResult{1, to_state_id}) : std::nullopt;
    }
    case CharacterClassType::AnyWordInverted:
    {
        auto c = input.at(0);
        bool condition = !((c >= U'a' && c <= U'z')
                        || (c >= U'A' && c <= U'Z')
                        || (c >= U'0' && c <= U'9')
                        || c == U'_');
        if (negate)
        {
            condition = !condition;
        }
        return condition ? std::make_optional<TransitionResult>(TransitionResult{1, to_state_id}) : std::nullopt;
    }
    case CharacterClassType::AnyWhitespace:
    {
        auto c = input.at(0);
        bool condition = (c == U' ' || c == U'\t' || c == U'\n' || c == U'\r' || c == U'\f' || c == U'\v');
        if (negate)
        {
            condition = !condition;
        }
        return condition ? std::make_optional<TransitionResult>(TransitionResult{1, to_state_id}) : std::nullopt;
    }
    case CharacterClassType::AnyWhitespaceInverted:
    {
        auto c = input.at(0);
        bool condition = !(c == U' ' || c == U'\t' || c == U'\n' || c == U'\r' || c == U'\f' || c == U'\v');
        if (negate)
        {
            condition = !condition;
        }
        return condition ? std::make_optional<TransitionResult>(TransitionResult{1, to_state_id}) : std::nullopt;
    }
    default:
        break;
    }
    return std::nullopt;
}

std::optional<TransitionResult> Matcher::process_character_range_transition(MatchState &match_state, StateId to_state_id, const CharacterRange &character_range, std::u32string_view input, bool negate)
{
    if (input.empty())
    {
        return std::nullopt;
    }
    auto c = input.at(0);
    bool condition = false;
    if (character_range.end.has_value())
    {
        condition = (c >= character_range.start && c <= character_range.end.value());
        if (negate)
        {
            condition = !condition;
        }
    }
    else
    {
        condition = (c == character_range.start);
        if (negate)
        {
            condition = !condition;
        }
    }
    return condition ? std::make_optional<TransitionResult>(TransitionResult{1, to_state_id}) : std::nullopt;
}

std::optional<TransitionResult> Matcher::process_unicode_category_name_transition(MatchState &match_state, StateId to_state_id, const UnicodeCategoryName &unicode_category_name, std::u32string_view input, bool negate)
{
    throw std::runtime_error("Unicode category name transition processing not implemented");
}

std::optional<TransitionResult> Matcher::process_character_group_transition(MatchState &match_state, StateId to_state_id, const CharacterGroup &character_group, std::u32string_view input)
{
    if (input.empty())
    {
        return std::nullopt;
    }

    struct Visitor
    {
        TransitionConditionType operator()(const CharacterClass& character_class) const
        {
            return character_class;
        }
        TransitionConditionType operator()(const CharacterClassFromUnicodeCategory& character_class_from_unicode_category) const
        {
            return character_class_from_unicode_category.category_name;
        }
        TransitionConditionType operator()(const CharacterRange& character_range) const
        {
            return character_range;
        }
        TransitionConditionType operator()(const Char& character) const
        {
            return character;
        }
    };
    Visitor visitor{};

    bool negate = character_group.negative_modifier.has_value();
    for (const auto& group_item : character_group.character_group_items)
    {
        auto transition_condition = std::visit(visitor, group_item.character_group_item);
        auto transition_result = process_transition(match_state, to_state_id, transition_condition, input, false);
        if (transition_result.has_value())
        {
            return negate ? std::nullopt : transition_result;
        }
    }
    return negate ? std::make_optional<TransitionResult>(TransitionResult{1, to_state_id}) : std::nullopt;
}

std::optional<TransitionResult> Matcher::process_anchor_transition(MatchState &match_state, StateId to_state_id, const Anchor &anchor, std::u32string_view input)
{
    auto is_word = [](char32_t c)
    {
        return ((c >= U'a' && c <= U'z')
             || (c >= U'A' && c <= U'Z')
             || (c >= U'0' && c <= U'9')
             || c == U'_');
    };
    switch (anchor.type)
    {
    case AnchorType::StartOfStringOnly:
    {
        if (match_state.input_position == 0)
        {
            return TransitionResult{0, to_state_id};
        }
        return std::nullopt;
    }
    case AnchorType::EndOfString:
    {
        if (match_state.input_position == match_state.original_input.size()
            || match_state.original_input[match_state.input_position] == U'\n')
        {
            return TransitionResult{0, to_state_id};
        }
        return std::nullopt;
    }
    case AnchorType::EndOfStringOnly:
    {
        if (match_state.input_position == match_state.original_input.size()
            || (match_state.input_position == match_state.original_input.size() - 1
                && match_state.original_input[match_state.input_position] == U'\n'))
        {
            return TransitionResult{0, to_state_id};
        }
        return std::nullopt;
    }
    case AnchorType::EndOfStringOnlyNotNewline:
    {
        if (match_state.input_position == match_state.original_input.size())
        {
            return TransitionResult{0, to_state_id};
        }
        return std::nullopt;
    }
    case AnchorType::NonWordBoundary:
    {
        std::optional<char32_t> prev_char{std::nullopt};
        std::optional<char32_t> next_char{std::nullopt};
        if (match_state.input_position > 0)
        {
            prev_char = match_state.original_input.at(match_state.input_position - 1);
        }
        if (match_state.input_position < match_state.original_input.size())
        {
            next_char = match_state.original_input.at(match_state.input_position);
        }
        bool is_prev_word = prev_char.has_value() && is_word(prev_char.value());
        bool is_next_word = next_char.has_value() && is_word(next_char.value());
        auto condition_result = (is_prev_word == is_next_word);
        if (condition_result)
        {
            return TransitionResult{0, to_state_id};
        }
        return std::nullopt;
    }
    case AnchorType::WordBoundary:
    {
        std::optional<char32_t> prev_char{std::nullopt};
        std::optional<char32_t> next_char{std::nullopt};
        if (match_state.input_position > 0)
        {
            prev_char = match_state.original_input.at(match_state.input_position - 1);
        }
        if (match_state.input_position < match_state.original_input.size())
        {
            next_char = match_state.original_input.at(match_state.input_position);
        }
        // if prev is nullopt, next is word
        // if prev is word, next is nullopt
        // if prev is word, next is non-word
        // if prev is non-word, next is word
        auto condition_result = (
            (!prev_char.has_value() && next_char.has_value() && is_word(next_char.value()))
            || (prev_char.has_value() && !next_char.has_value() && is_word(prev_char.value()))
            || (prev_char.has_value() && next_char.has_value() && is_word(prev_char.value()) != is_word(next_char.value())));
        
        if (condition_result)
        {
            return TransitionResult{0, to_state_id};
        }
        return std::nullopt;
    }
    case AnchorType::PreviousMatchEnd:
    {
        if (match_state.input_position == 0)
        {
            return TransitionResult{0, to_state_id};
        }
        return std::nullopt;
    }
    default:
        break;
    }
    return std::optional<TransitionResult>();
}

std::optional<TransitionResult> Matcher::process_start_of_string_anchor_transition(MatchState& match_state, StateId to_state_id, const StartOfStringAnchor& start_of_string_anchor, std::u32string_view input)
{
    return match_state.input_position == 0 ? std::make_optional<TransitionResult>(TransitionResult{0, to_state_id}) : std::nullopt;
}

std::optional<TransitionResult> Matcher::process_group_capture_start_transition(MatchState& match_state, StateId to_state_id, const GroupCaptureStart& group_capture_start, std::u32string_view input)
{
    if (!match_state.group_ranges)
    {
        match_state.group_ranges = std::make_shared<std::vector<Range>>();
    }
    auto it = std::find_if(
        match_state.group_ranges->begin(),
        match_state.group_ranges->end(),
        [&group_capture_start](const Range& group_range)
        {
            return group_range.group_id == group_capture_start.group_number;
        });
    if (it == match_state.group_ranges->end())
    {
        match_state.group_ranges->push_back(Range{group_capture_start.group_number, match_state.input_position, std::nullopt});
    }
    else
    {
        it->from_position = match_state.input_position;
        it->to_position = std::nullopt;
    }
    return TransitionResult{0, to_state_id};
}

std::optional<TransitionResult> Matcher::process_group_capture_end_transition(MatchState& match_state, StateId to_state_id, const GroupCaptureEnd& group_capture_end, std::u32string_view input)
{
    if (!match_state.group_ranges)
    {
        return std::nullopt;
    }
    auto it = std::find_if(
        match_state.group_ranges->begin(),
        match_state.group_ranges->end(),
        [&group_capture_end](const Range& group_range)
        {
            return group_range.group_id == group_capture_end.group_number;
        });
    if (it != match_state.group_ranges->end())
    {
        it->to_position = match_state.input_position;
    }
    else
    {
        return std::nullopt;
    }
    return TransitionResult{0, to_state_id};
}

std::optional<TransitionResult> Matcher::process_backreference_transition(MatchState& match_state, StateId to_state_id, const Backreference& backreference, std::u32string_view input)
{
    if (!match_state.group_ranges)
    {
        return std::nullopt;
    }
    auto it = std::find_if(
        match_state.group_ranges->begin(),
        match_state.group_ranges->end(),
        [&backreference](const Range& group_range)
        {
            return group_range.group_id == backreference;
        });
    if (it == match_state.group_ranges->end())
    {
        return std::nullopt;
    }
    const auto& group_range = *it;
    if (!group_range.from_position.has_value() || !group_range.to_position.has_value())
    {
        return std::nullopt;
    }
    std::size_t from_pos = group_range.from_position.value();
    std::size_t to_pos = group_range.to_position.value();
    auto view = match_state.original_input.substr(from_pos, to_pos - from_pos);
    if (input.starts_with(view))
    {
        return TransitionResult{view.size(), to_state_id};
    }
    return std::nullopt;
}

} // namespace kregex