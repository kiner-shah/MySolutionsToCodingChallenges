#ifndef K_REGEX_ENGINE_TEST_HELPERS_HPP
#define K_REGEX_ENGINE_TEST_HELPERS_HPP

#include <doctest.h>
#include "parser.hpp"

template<typename T>
void expect_ok(const kregex::ParseResult<T>& result, const kregex::ParseValue<T>& expected)
{
    REQUIRE(std::holds_alternative<kregex::ParseValue<T>>(result));
    REQUIRE(std::get<kregex::ParseValue<T>>(result) == expected);
}

template<typename T>
void expect_error(const kregex::ParseResult<T>& result)
{
    REQUIRE(std::holds_alternative<kregex::ParseError>(result));
}

#endif  // K_REGEX_ENGINE_TEST_HELPERS_HPP