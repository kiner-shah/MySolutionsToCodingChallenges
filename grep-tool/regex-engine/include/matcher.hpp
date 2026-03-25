#ifndef K_REGIX_ENGINE_MATCHER_HPP
#define K_REGIX_ENGINE_MATCHER_HPP

#include <memory>
#include <optional>
#include <string_view>
#include <vector>
#include "nfa.hpp"

namespace kregex
{
struct Range
{
    GroupId group_id;
    std::optional<std::size_t> from_position;
    std::optional<std::size_t> to_position;

    inline bool operator==(const Range& other) const
    {
        return group_id == other.group_id &&
               from_position == other.from_position &&
               to_position == other.to_position;
    }
    inline bool operator!=(const Range& other) const
    {
        return !(*this == other);
    }
    inline bool operator<(const Range& other) const
    {
        if (group_id != other.group_id)
        {
            return group_id < other.group_id;
        }
        if (from_position != other.from_position)
        {
            return from_position < other.from_position;
        }
        return to_position < other.to_position;
    }
};

struct MatchResult
{
    // 0th index is the whole match, subsequent indices are capturing groups
    std::vector<Range> group_ranges;
};

struct MatchState
{
    std::u32string_view original_input;
    std::size_t input_position;
    StateId current_state_id;
    std::shared_ptr<std::vector<Range>> group_ranges;
};

struct TransitionResult
{
    std::size_t offset;
    StateId to_state_id;
};

class Matcher
{
    NfaElement m_regex_nfa_element;
    std::vector<State> m_states;

    std::optional<TransitionResult> process_char_transition(
        MatchState& match_state,
        StateId to_state_id,
        const Char& character,
        std::u32string_view input,
        bool negate = false);
    std::optional<TransitionResult> process_character_class_transition(
        MatchState& match_state,
        StateId to_state_id,
        const CharacterClass& character_class,
        std::u32string_view input,
        bool negate = false);
    std::optional<TransitionResult> process_character_range_transition(
        MatchState& match_state,
        StateId to_state_id,
        const CharacterRange& character_range,
        std::u32string_view input,
        bool negate = false);
    std::optional<TransitionResult> process_unicode_category_name_transition(
        MatchState& match_state,
        StateId to_state_id,
        const UnicodeCategoryName& unicode_category_name,
        std::u32string_view input,
        bool negate = false);
    std::optional<TransitionResult> process_character_group_transition(
        MatchState& match_state,
        StateId to_state_id,
        const CharacterGroup& character_group,
        std::u32string_view input);
    std::optional<TransitionResult> process_anchor_transition(
        MatchState& match_state,
        StateId to_state_id,
        const Anchor& anchor,
        std::u32string_view input);
    std::optional<TransitionResult> process_start_of_string_anchor_transition(
        MatchState& match_state,
        StateId to_state_id,
        const StartOfStringAnchor& start_of_string_anchor,
        std::u32string_view input);
    std::optional<TransitionResult> process_group_capture_start_transition(
        MatchState& match_state,
        StateId to_state_id,
        const GroupCaptureStart& group_capture_start,
        std::u32string_view input);
    std::optional<TransitionResult> process_group_capture_end_transition(
        MatchState& match_state,
        StateId to_state_id,
        const GroupCaptureEnd& group_capture_end,
        std::u32string_view input);
    std::optional<TransitionResult> process_backreference_transition(
        MatchState& match_state,
        StateId to_state_id,
        const Backreference& backreference,
        std::u32string_view input);
public:
    Matcher(const NfaElement& regex_nfa_element, const std::vector<State>& states);
    std::optional<TransitionResult> process_transition(
        MatchState& match_state,
        StateId to_state_id,
        const TransitionConditionType& transition_condition,
        std::u32string_view input,
        bool negate = false);
    std::optional<MatchResult> match_one(std::u32string_view input);
    std::vector<MatchResult> match_all(std::u32string_view input);
};
}   // namespace kregex

#endif  // K_REGIX_ENGINE_MATCHER_HPP