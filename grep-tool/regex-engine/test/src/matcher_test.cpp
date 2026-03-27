#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "test_helpers.hpp"

#include "matcher.hpp"

using namespace kregex;

namespace
{
Regex parse_regex_or_fail(std::string_view regex)
{
	auto result = parse_regex(regex);
	REQUIRE(std::holds_alternative<ParseValue<Regex>>(result));
	return std::get<ParseValue<Regex>>(result).value;
}

Matcher build_matcher(std::string_view regex)
{
	Nfa nfa;
	NfaElement element = nfa.build_regex(parse_regex_or_fail(regex));
	return Matcher{element, nfa.get_states()};
}
}

TEST_CASE("Matcher literal succeeds on prefix")
{
	auto matcher = build_matcher("ab");
	auto result = matcher.match_one(U"abc");

	REQUIRE(result.has_value());
	REQUIRE(result->group_ranges.size() == 1);
	CHECK(result->group_ranges[0] == Range{0, 0, 2});
}

TEST_CASE("Matcher literal fails when first character mismatches")
{
	auto matcher = build_matcher("ab");
	auto result = matcher.match_one(U"zab");
	CHECK_FALSE(result.has_value());
}

TEST_CASE("Matcher character class matches decimal digits")
{
	auto matcher = build_matcher("\\d\\d");
	auto result = matcher.match_one(U"42x");

	REQUIRE(result.has_value());
	CHECK(result->group_ranges[0] == Range{0, 0, 2});
}

TEST_CASE("Matcher character class matches whitespace and non-whitespace")
{
	auto whitespace_matcher = build_matcher("\\s");
	auto non_whitespace_matcher = build_matcher("\\S");

	CHECK(whitespace_matcher.match_one(U" ").has_value());
	CHECK(whitespace_matcher.match_one(U"\t").has_value());
	CHECK(whitespace_matcher.match_one(U"\n").has_value());
	CHECK_FALSE(whitespace_matcher.match_one(U"a").has_value());

	CHECK(non_whitespace_matcher.match_one(U"a").has_value());
	CHECK(non_whitespace_matcher.match_one(U"9").has_value());
	CHECK_FALSE(non_whitespace_matcher.match_one(U" ").has_value());
	CHECK_FALSE(non_whitespace_matcher.match_one(U"\n").has_value());
}

TEST_CASE("Matcher negated character group rejects in-range and accepts out-of-range")
{
	auto matcher = build_matcher("[^a-z]");

	auto fail_result = matcher.match_one(U"k");
	CHECK_FALSE(fail_result.has_value());

	auto pass_result = matcher.match_one(U"7");
	REQUIRE(pass_result.has_value());
	CHECK(pass_result->group_ranges[0] == Range{0, 0, 1});
}

TEST_CASE("Matcher captures group ranges")
{
	auto matcher = build_matcher("(ab)");
	auto result = matcher.match_one(U"ab");

	REQUIRE(result.has_value());
	REQUIRE(result->group_ranges.size() == 2);
	CHECK(result->group_ranges[0] == Range{0, 0, 2});
	CHECK(result->group_ranges[1] == Range{1, 0, 2});
}

TEST_CASE("Matcher backreference succeeds for repeated capture")
{
	auto matcher = build_matcher("(ab)\\1");
	auto result = matcher.match_one(U"abab");

	REQUIRE(result.has_value());
	REQUIRE(result->group_ranges.size() == 2);
	CHECK(result->group_ranges[0] == Range{0, 0, 4});
	CHECK(result->group_ranges[1] == Range{1, 0, 2});
}

TEST_CASE("Matcher backreference fails when captured value differs")
{
	auto matcher = build_matcher("(ab)\\1");
	auto result = matcher.match_one(U"abac");
	CHECK_FALSE(result.has_value());
}

TEST_CASE("Matcher start-of-string anchor only matches at beginning")
{
	auto matcher = build_matcher("^ab");

	CHECK(matcher.match_one(U"abxx").has_value());
	CHECK_FALSE(matcher.match_one(U"zab").has_value());
}

TEST_CASE("Matcher word boundary anchors")
{
	auto matcher = build_matcher("\\bcat\\b");

	CHECK(matcher.match_one(U"cat ").has_value());
	CHECK_FALSE(matcher.match_one(U"cat1").has_value());
}

TEST_CASE("Matcher non-word-boundary anchor")
{
	auto matcher = build_matcher(".\\Bcat\\B");

	CHECK(matcher.match_one(U"scat1").has_value());
	CHECK_FALSE(matcher.match_one(U"scat ").has_value());
}

TEST_CASE("Matcher end-of-string anchor accepts terminal newline behavior")
{
	auto matcher = build_matcher("ab$");

	CHECK(matcher.match_one(U"ab").has_value());
	CHECK(matcher.match_one(U"ab\n").has_value());
	CHECK_FALSE(matcher.match_one(U"abx").has_value());
}

TEST_CASE("Matcher match_all returns non-overlapping matches")
{
	auto matcher = build_matcher("ab");
	auto results = matcher.match_all(U"zababx");

	REQUIRE(results.size() == 2);
	CHECK(results[0].group_ranges[0] == Range{0, 0, 2});
	CHECK(results[1].group_ranges[0] == Range{0, 0, 2});
}

TEST_CASE("Matcher backreference works with alternation captures")
{
	auto matcher = build_matcher("(ab|cd)\\1");

	CHECK(matcher.match_one(U"abab").has_value());
	CHECK(matcher.match_one(U"cdcd").has_value());
	CHECK_FALSE(matcher.match_one(U"abcd").has_value());
}

TEST_CASE("Matcher nested capture can backreference inner group")
{
	auto matcher = build_matcher("((a))\\2");
	auto result = matcher.match_one(U"aa");

	REQUIRE(result.has_value());
	REQUIRE(result->group_ranges.size() == 3);
	CHECK(result->group_ranges[0] == Range{0, 0, 2});
	CHECK(result->group_ranges[1] == Range{1, 0, 1});
	CHECK(result->group_ranges[2] == Range{2, 0, 1});
}

TEST_CASE("Matcher quantified capture updates to latest occurrence")
{
	auto matcher = build_matcher("(a)+\\1");

	CHECK(matcher.match_one(U"aa").has_value());
	CHECK(matcher.match_one(U"aaa").has_value());
}

TEST_CASE("Matcher match_all progresses for zero-width matches")
{
	auto matcher = build_matcher("a*");
	auto results = matcher.match_all(U"bbb");

	REQUIRE(results.size() == 3);
	for (const auto& result : results)
	{
		REQUIRE(!result.group_ranges.empty());
		CHECK(result.group_ranges[0] == Range{0, 0, 0});
	}
}
