#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "test_helpers.hpp"
#include <algorithm>
#include <memory>
#include <type_traits>
#include <vector>

using namespace kregex;

namespace
{
Regex parse_regex_or_fail(std::string_view regex)
{
	auto result = parse_regex(regex);
	REQUIRE(std::holds_alternative<ParseValue<Regex>>(result));
	return std::get<ParseValue<Regex>>(result).value;
}

template<typename T>
std::size_t count_transitions_of_type(const std::vector<State>& states)
{
	std::size_t count = 0;
	for (const auto& state : states)
	{
		for (const auto& transition : state.transitions)
		{
			if (std::holds_alternative<T>(transition.condition))
			{
				++count;
			}
		}
	}
	return count;
}

template<typename T>
std::vector<T> collect_transition_values(const std::vector<State>& states)
{
	std::vector<T> values;
	for (const auto& state : states)
	{
		for (const auto& transition : state.transitions)
		{
			if (const auto* value = std::get_if<T>(&transition.condition))
			{
				values.push_back(*value);
			}
		}
	}
	return values;
}

std::size_t count_transitions(const std::vector<State>& states)
{
	std::size_t count = 0;
	for (const auto& state : states)
	{
		count += state.transitions.size();
	}
	return count;
}

template<typename T>
bool state_has_transition_type(const std::vector<State>& states, StateId state_id)
{
	for (const auto& transition : states[state_id].transitions)
	{
		if (std::holds_alternative<T>(transition.condition))
		{
			return true;
		}
	}
	return false;
}
}  // namespace

TEST_CASE("Build literal regex into NFA")
{
	Nfa nfa;
	NfaElement element = nfa.build_regex(parse_regex_or_fail("a"));
	const auto& states = nfa.get_states();

	CHECK(states.size() == 4);
	CHECK(count_transitions(states) == 3);
	CHECK(element.start == 0);
	CHECK(element.accept == 1);
	CHECK(count_transitions_of_type<Char>(states) == 1);
	CHECK(count_transitions_of_type<Epsilon>(states) == 2);

	const auto chars = collect_transition_values<Char>(states);
	REQUIRE(chars.size() == 1);
	CHECK(chars.front() == U'a');
}

TEST_CASE("Build concatenated regex into NFA")
{
	Nfa nfa;
	nfa.build_regex(parse_regex_or_fail("ab"));
	const auto& states = nfa.get_states();

	CHECK(states.size() == 6);
	CHECK(count_transitions(states) == 5);
	CHECK(count_transitions_of_type<Char>(states) == 2);
	CHECK(count_transitions_of_type<Epsilon>(states) == 3);

	const auto chars = collect_transition_values<Char>(states);
	REQUIRE(chars.size() == 2);
	CHECK(chars[0] == U'a');
	CHECK(chars[1] == U'b');
}

TEST_CASE("Build regex with start of string anchor into NFA")
{
	Nfa nfa;
	NfaElement element = nfa.build_regex(parse_regex_or_fail("^a"));
	const auto& states = nfa.get_states();

	CHECK(states.size() == 6);
	CHECK(count_transitions(states) == 5);
	CHECK(element.start == 0);
	CHECK(element.accept == 3);

	CHECK(count_transitions_of_type<StartOfStringAnchor>(states) == 1);
	CHECK(count_transitions_of_type<Char>(states) == 1);
	CHECK(count_transitions_of_type<Epsilon>(states) == 3);

	const auto chars = collect_transition_values<Char>(states);
	REQUIRE(chars.size() == 1);
	CHECK(chars.front() == U'a');
}

TEST_CASE("Build capturing group emits capture transitions")
{
	Nfa nfa;
	nfa.build_regex(parse_regex_or_fail("(a)"));
	const auto& states = nfa.get_states();

	CHECK(states.size() == 8);
	CHECK(count_transitions(states) == 7);

	const auto capture_starts = collect_transition_values<GroupCaptureStart>(states);
	const auto capture_ends = collect_transition_values<GroupCaptureEnd>(states);

	REQUIRE(capture_starts.size() == 1);
	REQUIRE(capture_ends.size() == 1);
	CHECK(capture_starts.front().group_number == 1);
	CHECK(capture_ends.front().group_number == 1);
}

TEST_CASE("Quantified capturing group reuses the same capture group number")
{
	Nfa nfa;
	nfa.build_regex(parse_regex_or_fail("(a){2}"));
	const auto& states = nfa.get_states();
	CHECK(states.size() == 14);
	CHECK(count_transitions(states) == 13);

	const auto capture_starts = collect_transition_values<GroupCaptureStart>(states);
	const auto capture_ends = collect_transition_values<GroupCaptureEnd>(states);

	REQUIRE(capture_starts.size() == 2);
	REQUIRE(capture_ends.size() == 2);
	CHECK(std::all_of(capture_starts.begin(), capture_starts.end(), [](const GroupCaptureStart& value) {
		return value.group_number == 1;
	}));
	CHECK(std::all_of(capture_ends.begin(), capture_ends.end(), [](const GroupCaptureEnd& value) {
		return value.group_number == 1;
	}));
}

TEST_CASE("Non-capturing group does not emit capture transitions")
{
	Nfa nfa;
	nfa.build_regex(parse_regex_or_fail("(?:a)"));
	const auto& states = nfa.get_states();

	CHECK(states.size() == 6);
	CHECK(count_transitions(states) == 5);
	CHECK(count_transitions_of_type<GroupCaptureStart>(states) == 0);
	CHECK(count_transitions_of_type<GroupCaptureEnd>(states) == 0);
	CHECK(count_transitions_of_type<Char>(states) == 1);
}

TEST_CASE("Build backreference into NFA")
{
	Nfa nfa;
	nfa.build_regex(parse_regex_or_fail("(a)\\1"));
	const auto& states = nfa.get_states();
	CHECK(states.size() == 10);
	CHECK(count_transitions(states) == 9);

	const auto backreferences = collect_transition_values<Backreference>(states);
	REQUIRE(backreferences.size() == 1);
	CHECK(backreferences.front() == 1);
}

TEST_CASE("Build regex with quantifiers")
{
    auto quantifier_check = [](std::string_view regex,
                                    std::size_t expected_states,
                                    std::size_t expected_transitions,
                                    std::size_t expected_char_transitions,
                                    std::size_t expected_epsilon_transitions)
    {
        Nfa nfa;
        nfa.build_regex(parse_regex_or_fail(regex));
        const auto& states = nfa.get_states();
        CHECK(states.size() == expected_states);
        CHECK(count_transitions(states) == expected_transitions);
        CHECK(count_transitions_of_type<Char>(states) == expected_char_transitions);
        CHECK(count_transitions_of_type<Epsilon>(states) == expected_epsilon_transitions);
    };
    quantifier_check("a*", 6, 7, 1, 6);
    quantifier_check("a+", 5, 5, 1, 4);
	quantifier_check("a+?", 5, 5, 1, 4);
    quantifier_check("a?", 6, 6, 1, 5);
	quantifier_check("a??", 6, 6, 1, 5);
}

TEST_CASE("Lazy modifier changes epsilon branch order")
{
	Nfa greedy_nfa;
	NfaElement greedy = greedy_nfa.build_regex(parse_regex_or_fail("a*"));
	const auto& greedy_states = greedy_nfa.get_states();
	const StateId greedy_split = greedy_states[greedy.start].transitions.front().to_state;

	REQUIRE(greedy_states[greedy_split].transitions.size() == 2);
	CHECK(std::holds_alternative<Epsilon>(greedy_states[greedy_split].transitions[0].condition));
	CHECK(std::holds_alternative<Epsilon>(greedy_states[greedy_split].transitions[1].condition));
	CHECK(state_has_transition_type<Char>(greedy_states, greedy_states[greedy_split].transitions[0].to_state));
	CHECK_FALSE(state_has_transition_type<Char>(greedy_states, greedy_states[greedy_split].transitions[1].to_state));

	Nfa lazy_nfa;
	NfaElement lazy = lazy_nfa.build_regex(parse_regex_or_fail("a*?"));
	const auto& lazy_states = lazy_nfa.get_states();
	const StateId lazy_split = lazy_states[lazy.start].transitions.front().to_state;

	REQUIRE(lazy_states[lazy_split].transitions.size() == 2);
	CHECK(std::holds_alternative<Epsilon>(lazy_states[lazy_split].transitions[0].condition));
	CHECK(std::holds_alternative<Epsilon>(lazy_states[lazy_split].transitions[1].condition));
	CHECK_FALSE(state_has_transition_type<Char>(lazy_states, lazy_states[lazy_split].transitions[0].to_state));
	CHECK(state_has_transition_type<Char>(lazy_states, lazy_states[lazy_split].transitions[1].to_state));
}

TEST_CASE("Build regex with range quantifiers")
{
    auto range_quantifier_check = [](std::string_view regex,
                                    std::size_t expected_states,
                                    std::size_t expected_transitions,
                                    std::size_t expected_char_transitions,
                                    std::size_t expected_epsilon_transitions)
    {
        Nfa nfa;
        nfa.build_regex(parse_regex_or_fail(regex));
        const auto& states = nfa.get_states();
        CHECK(states.size() == expected_states);
        CHECK(count_transitions(states) == expected_transitions);
        CHECK(count_transitions_of_type<Char>(states) == expected_char_transitions);
        CHECK(count_transitions_of_type<Epsilon>(states) == expected_epsilon_transitions);
    };
	range_quantifier_check("a{2,3}", 10, 10, 3, 7);
	range_quantifier_check("a{2,3}?", 10, 10, 3, 7);
	range_quantifier_check("a{2,}", 10, 11, 3, 8);
	range_quantifier_check("a{2}", 6, 5, 2, 3);
	range_quantifier_check("a{0,2}", 12, 13, 2, 11);
	range_quantifier_check("a{0,}", 6, 7, 1, 6);
	range_quantifier_check("a{1,2}", 8, 8, 2, 6);
	range_quantifier_check("a{1,}", 8, 9, 2, 7);
}

TEST_CASE("Build regex with alternations")
{
    Nfa nfa;
    nfa.build_regex(parse_regex_or_fail("a|b"));
    const auto& states = nfa.get_states();
    CHECK(states.size() == 6);
    CHECK(count_transitions(states) == 6);
    CHECK(count_transitions_of_type<Char>(states) == 2);
    CHECK(count_transitions_of_type<Epsilon>(states) == 4);
    const auto chars = collect_transition_values<Char>(states);
    REQUIRE(chars.size() == 2);
    CHECK(chars[0] == U'a');
    CHECK(chars[1] == U'b');
}

TEST_CASE("Build regex with multiple capturing groups")
{
	Nfa nfa;
	nfa.build_regex(parse_regex_or_fail("(a)(b)"));
	const auto& states = nfa.get_states();

	CHECK(states.size() == 14);
	CHECK(count_transitions(states) == 13);
	CHECK(count_transitions_of_type<Char>(states) == 2);
	CHECK(count_transitions_of_type<GroupCaptureStart>(states) == 2);
	CHECK(count_transitions_of_type<GroupCaptureEnd>(states) == 2);
	CHECK(count_transitions_of_type<Epsilon>(states) == 7);

	const auto capture_starts = collect_transition_values<GroupCaptureStart>(states);
	REQUIRE(capture_starts.size() == 2);
	CHECK(capture_starts[0].group_number == 1);
	CHECK(capture_starts[1].group_number == 2);
}

TEST_CASE("Build regex with nested capturing groups")
{
	Nfa nfa;
	nfa.build_regex(parse_regex_or_fail("((a))"));
	const auto& states = nfa.get_states();

	CHECK(states.size() == 12);
	CHECK(count_transitions(states) == 11);
	CHECK(count_transitions_of_type<Char>(states) == 1);
	CHECK(count_transitions_of_type<GroupCaptureStart>(states) == 2);
	CHECK(count_transitions_of_type<GroupCaptureEnd>(states) == 2);
	CHECK(count_transitions_of_type<Epsilon>(states) == 6);

	const auto capture_starts = collect_transition_values<GroupCaptureStart>(states);
	REQUIRE(capture_starts.size() == 2);
	CHECK(capture_starts[0].group_number == 1);
	CHECK(capture_starts[1].group_number == 2);
}

TEST_CASE("Build regex with nested non-capturing and capturing groups")
{
	Nfa nfa;
	nfa.build_regex(parse_regex_or_fail("(?:a(b))"));
	const auto& states = nfa.get_states();

	CHECK(states.size() == 12);
	CHECK(count_transitions(states) == 11);
	CHECK(count_transitions_of_type<Char>(states) == 2);
	CHECK(count_transitions_of_type<GroupCaptureStart>(states) == 1);
	CHECK(count_transitions_of_type<GroupCaptureEnd>(states) == 1);
	CHECK(count_transitions_of_type<Epsilon>(states) == 7);

	const auto capture_starts = collect_transition_values<GroupCaptureStart>(states);
	REQUIRE(capture_starts.size() == 1);
	CHECK(capture_starts.front().group_number == 1);
}

TEST_CASE("Build regex with negated character group")
{
	Nfa nfa;
	nfa.build_regex(parse_regex_or_fail("[^a-z]"));
	const auto& states = nfa.get_states();

	CHECK(states.size() == 4);
	CHECK(count_transitions(states) == 3);
	CHECK(count_transitions_of_type<CharacterGroup>(states) == 1);
	CHECK(count_transitions_of_type<Epsilon>(states) == 2);

	const auto groups = collect_transition_values<CharacterGroup>(states);
	REQUIRE(groups.size() == 1);
	CHECK(groups.front().negative_modifier.has_value());
	CHECK(groups.front().character_group_items.size() == 1);
}

TEST_CASE("Build regex with dot wildcard")
{
	Nfa nfa;
	nfa.build_regex(parse_regex_or_fail("."));
	const auto& states = nfa.get_states();

	CHECK(states.size() == 4);
	CHECK(count_transitions(states) == 3);
	CHECK(count_transitions_of_type<MatchAnyCharacter>(states) == 1);
	CHECK(count_transitions_of_type<Epsilon>(states) == 2);
}

TEST_CASE("Build regex with character class")
{
    Nfa nfa;
    nfa.build_regex(parse_regex_or_fail("\\d"));
    const auto& states = nfa.get_states();

    CHECK(states.size() == 4);
    CHECK(count_transitions(states) == 3);
    CHECK(count_transitions_of_type<CharacterClass>(states) == 1);
    CHECK(count_transitions_of_type<Epsilon>(states) == 2);

    const auto classes = collect_transition_values<CharacterClass>(states);
    REQUIRE(classes.size() == 1);
    CHECK(classes.front().type == CharacterClassType::AnyDecimalDigit);
}