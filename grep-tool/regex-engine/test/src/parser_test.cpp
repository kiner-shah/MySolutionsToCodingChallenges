#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "test_helpers.hpp"

using namespace kregex;

TEST_CASE("Parse letters")
{
    expect_ok(parse_letters("abc"), ParseValue<Letters>{"abc", ""});
    expect_ok(parse_letters("abc123"), ParseValue<Letters>{"abc", "123"});
    expect_error(parse_letters("123"));
}

TEST_CASE("Parse number")
{
    expect_ok(parse_number("123"), ParseValue<Integer>{123, ""});
    expect_ok(parse_number("0"), ParseValue<Integer>{0, ""});
    expect_ok(parse_number("123abc"), ParseValue<Integer>{123, "abc"});
    expect_error(parse_number("abc"));
}

TEST_CASE("Parse range qualifier")
{
    expect_ok(parse_range_quantifier("{3}"), ParseValue<RangeQuantifier>{RangeQuantifier{true, 3, std::nullopt}, ""});
    expect_ok(parse_range_quantifier("{3,}"), ParseValue<RangeQuantifier>{RangeQuantifier{false, 3, std::nullopt}, ""});
    expect_ok(parse_range_quantifier("{3,5}"), ParseValue<RangeQuantifier>{RangeQuantifier{false, 3, 5}, ""});
    expect_ok(parse_range_quantifier("{3,5}abc"), ParseValue<RangeQuantifier>{RangeQuantifier{false, 3, 5}, "abc"});
    expect_error(parse_range_quantifier("3,5}"));
    expect_error(parse_range_quantifier("{3,5"));
    expect_error(parse_range_quantifier("{,5}"));
    expect_error(parse_range_quantifier("{3,2}"));
    expect_error(parse_range_quantifier("{3,5x}"));
}

TEST_CASE("Parse quantifier")
{
    expect_ok(parse_quantifier("*"), ParseValue<Quantifier>{Quantifier{QuantifierType::ZeroOrMore, std::nullopt, std::nullopt}, ""});
    expect_ok(parse_quantifier("+"), ParseValue<Quantifier>{Quantifier{QuantifierType::OneOrMore, std::nullopt, std::nullopt}, ""});
    expect_ok(parse_quantifier("?"), ParseValue<Quantifier>{Quantifier{QuantifierType::ZeroOrOne, std::nullopt, std::nullopt}, ""});
    expect_ok(parse_quantifier("*?"), ParseValue<Quantifier>{Quantifier{QuantifierType::ZeroOrMore, std::nullopt, LazyModifier{}}, ""});
    expect_ok(parse_quantifier("+?"), ParseValue<Quantifier>{Quantifier{QuantifierType::OneOrMore, std::nullopt, LazyModifier{}}, ""});
    expect_ok(parse_quantifier("??"), ParseValue<Quantifier>{Quantifier{QuantifierType::ZeroOrOne, std::nullopt, LazyModifier{}}, ""});
    expect_ok(parse_quantifier("{3}"), ParseValue<Quantifier>{Quantifier{QuantifierType::Range, RangeQuantifier{true, 3, std::nullopt}, std::nullopt}, ""});
    expect_ok(parse_quantifier("{3}?"), ParseValue<Quantifier>{Quantifier{QuantifierType::Range, RangeQuantifier{true, 3, std::nullopt}, LazyModifier{}}, ""});
    expect_error(parse_quantifier(""));
    expect_error(parse_quantifier("x"));
}

TEST_CASE("Parse char")
{
    expect_ok(parse_char("a"), ParseValue<Char>{U'a', ""});
    expect_ok(parse_char("1"), ParseValue<Char>{U'1', ""});
    expect_ok(parse_char("あ"), ParseValue<Char>{U'あ', ""});
    expect_ok(parse_char("あa"), ParseValue<Char>{U'あ', "a"});
    expect_ok(parse_char("आम"), ParseValue<Char>{U'आ', "म"});
    expect_ok(parse_char("😃abc"), ParseValue<Char>{U'😃', "abc"});
    expect_error(parse_char(""));
}

TEST_CASE("Parse character class from unicode category")
{
    expect_ok(parse_character_class_from_unicode_category("\\p{Lt}"), ParseValue<CharacterClassFromUnicodeCategory>{UnicodeCategoryName{"Lt"}, ""});
    expect_error(parse_character_class_from_unicode_category("\\p{Lt"));
}

TEST_CASE("Parse character class")
{
    expect_ok(parse_character_class("\\d"), ParseValue<CharacterClass>{CharacterClass{CharacterClassType::AnyDecimalDigit}, ""});
    expect_ok(parse_character_class("\\D"), ParseValue<CharacterClass>{CharacterClass{CharacterClassType::AnyDecimalDigitInverted}, ""});
    expect_ok(parse_character_class("\\w"), ParseValue<CharacterClass>{CharacterClass{CharacterClassType::AnyWord}, ""});
    expect_ok(parse_character_class("\\W"), ParseValue<CharacterClass>{CharacterClass{CharacterClassType::AnyWordInverted}, ""});
    expect_error(parse_character_class("d"));
    expect_error(parse_character_class("\\x"));
    expect_error(parse_character_class("\\"));
}

TEST_CASE("Parse character range")
{
    expect_ok(parse_character_range("a-z"), ParseValue<CharacterRange>{CharacterRange{U'a', U'z'}, ""});
    expect_ok(parse_character_range("0-9"), ParseValue<CharacterRange>{CharacterRange{U'0', U'9'}, ""});
    expect_ok(parse_character_range("あ-お"), ParseValue<CharacterRange>{CharacterRange{U'あ', U'お'}, ""});
    expect_ok(parse_character_range("a-zx"), ParseValue<CharacterRange>{CharacterRange{U'a', U'z'}, "x"});
    expect_ok(parse_character_range("a"), ParseValue<CharacterRange>{CharacterRange{U'a', std::nullopt}, ""});
    expect_error(parse_character_range("a-"));
    expect_error(parse_character_range("-a"));
    expect_error(parse_character_range("z-a"));
}

TEST_CASE("Parse character group")
{
    expect_ok(parse_character_group("[a-z0-9]"), ParseValue<CharacterGroup>{
        CharacterGroup{
            std::nullopt,
            std::vector<CharacterGroupItem>{
                CharacterGroupItem{CharacterRange{U'a', U'z'}},
                CharacterGroupItem{CharacterRange{U'0', U'9'}}
            }
        },
        ""
    });
    expect_ok(parse_character_group("[^a-z]"), ParseValue<CharacterGroup>{
        CharacterGroup{
            CharacterGroupNegativeModifier{},
            std::vector<CharacterGroupItem>{
                CharacterGroupItem{CharacterRange{U'a', U'z'}}
            }
        },
        ""
    });
    expect_error(parse_character_group("a-z]"));
    expect_error(parse_character_group("[a-z"));
}

TEST_CASE("Parse match character class")
{
    expect_ok(parse_match_character_class("[a-z]"), ParseValue<MatchCharacterClass>{MatchCharacterClass{CharacterGroup{std::nullopt, std::vector<CharacterGroupItem>{CharacterGroupItem{CharacterRange{U'a', U'z'}}}}}, ""});
    expect_ok(parse_match_character_class("\\d"), ParseValue<MatchCharacterClass>{MatchCharacterClass{CharacterClass{CharacterClassType::AnyDecimalDigit}}, ""});
    expect_ok(parse_match_character_class("\\p{Lt}"), ParseValue<MatchCharacterClass>{MatchCharacterClass{CharacterClassFromUnicodeCategory{UnicodeCategoryName{"Lt"}}}, ""});
    expect_error(parse_match_character_class("x"));
}

TEST_CASE("Parse match item")
{
    expect_ok(parse_match_item("[a-z]"), ParseValue<MatchItem>{MatchItem{MatchCharacterClass{CharacterGroup{std::nullopt, std::vector<CharacterGroupItem>{CharacterGroupItem{CharacterRange{U'a', U'z'}}}}}}, ""});
    expect_ok(parse_match_item("\\d"), ParseValue<MatchItem>{MatchItem{MatchCharacterClass{CharacterClass{CharacterClassType::AnyDecimalDigit}}}, ""});
    expect_ok(parse_match_item("\\p{Lt}"), ParseValue<MatchItem>{MatchItem{MatchCharacterClass{CharacterClassFromUnicodeCategory{UnicodeCategoryName{"Lt"}}}}, ""});
    expect_ok(parse_match_item("a"), ParseValue<MatchItem>{MatchItem{MatchCharacter{U'a'}}, ""});
    expect_ok(parse_match_item(".abc"), ParseValue<MatchItem>{MatchItem{MatchAnyCharacter{}}, "abc"});
    expect_error(parse_match_item(""));
}

TEST_CASE("Parse anchor")
{
    expect_ok(parse_anchor("\\b"), ParseValue<Anchor>{Anchor{AnchorType::WordBoundary}, ""});
    expect_ok(parse_anchor("\\B"), ParseValue<Anchor>{Anchor{AnchorType::NonWordBoundary}, ""});
    expect_ok(parse_anchor("\\A"), ParseValue<Anchor>{Anchor{AnchorType::StartOfStringOnly}, ""});
    expect_ok(parse_anchor("$"), ParseValue<Anchor>{Anchor{AnchorType::EndOfString}, ""});
    expect_ok(parse_anchor("\\Z"), ParseValue<Anchor>{Anchor{AnchorType::EndOfStringOnly}, ""});
    expect_ok(parse_anchor("\\z"), ParseValue<Anchor>{Anchor{AnchorType::EndOfStringOnlyNotNewline}, ""});
    expect_ok(parse_anchor("\\G"), ParseValue<Anchor>{Anchor{AnchorType::PreviousMatchEnd}, ""});
    expect_error(parse_anchor("x"));
    expect_error(parse_anchor("\\"));
}

TEST_CASE("Parse backreference")
{
    expect_ok(parse_backreference("\\1"), ParseValue<Backreference>{Backreference{1}, ""});
    expect_ok(parse_backreference("\\10"), ParseValue<Backreference>{Backreference{10}, ""});
    expect_ok(parse_backreference("\\1a"), ParseValue<Backreference>{Backreference{1}, "a"});
    expect_error(parse_backreference("\\"));
    expect_error(parse_backreference("\\x"));
}

TEST_CASE("Parse sub-expression boundaries")
{
    // Delimiters are not valid sub-expression item starts.
    expect_error(parse_sub_expression_item("|a"));
    expect_error(parse_sub_expression_item(")a"));

    // Sub-expression should stop before '|'.
    expect_ok(parse_sub_expression("a|b"), ParseValue<SubExpression>{
        SubExpression{
            std::vector<SubExpressionItem>{
                SubExpressionItem{Match{MatchItem{MatchCharacter{U'a'}}, std::nullopt}}
            }
        },
        "|b"
    });

    // Sub-expression should stop before ')'.
    expect_ok(parse_sub_expression("a)bc"), ParseValue<SubExpression>{
        SubExpression{
            std::vector<SubExpressionItem>{
                SubExpressionItem{Match{MatchItem{MatchCharacter{U'a'}}, std::nullopt}}
            }
        },
        ")bc"
    });
}

TEST_CASE("Parse expression alternation")
{
    expect_ok(parse_expression("a|b"), ParseValue<Expression>{
       Expression{
           std::vector<SubExpression>{
               SubExpression{std::vector<SubExpressionItem>{SubExpressionItem{Match{MatchItem{MatchCharacter{U'a'}}, std::nullopt}}}},
               SubExpression{std::vector<SubExpressionItem>{SubExpressionItem{Match{MatchItem{MatchCharacter{U'b'}}, std::nullopt}}}}
           }
       },
       ""
    });
    expect_error(parse_expression("|a"));
    expect_error(parse_expression("a|"));
}

TEST_CASE("Parse group")
{
    expect_ok(parse_group("(ab)c"), ParseValue<Group>{
        Group {
            std::nullopt,
            std::make_shared<Expression>(Expression{
                std::vector<SubExpression>{
                    SubExpression{
                        std::vector<SubExpressionItem>{
                            SubExpressionItem{Match{MatchItem{MatchCharacter{U'a'}}, std::nullopt}},
                            SubExpressionItem{Match{MatchItem{MatchCharacter{U'b'}}, std::nullopt}}
                        }
                    }
                }
            }),
            std::nullopt
        },
        "c"
    });
    expect_error(parse_group("(ab"));
}

TEST_CASE("Parse Regex")
{
    expect_ok(parse_regex("([a-z0-9]+)"), ParseValue<Regex>{
        Regex{
            std::nullopt,
            Expression{
                std::vector<SubExpression>{
                    SubExpression{
                        std::vector<SubExpressionItem>{
                            SubExpressionItem{
                                Group{
                                    std::nullopt,
                                    std::make_shared<Expression>(Expression{
                                        std::vector<SubExpression>{
                                            SubExpression{
                                                std::vector<SubExpressionItem>{
                                                    SubExpressionItem{
                                                        Match{
                                                            MatchItem{
                                                                MatchCharacterClass{
                                                                    CharacterGroup{
                                                                        std::nullopt,
                                                                        std::vector<CharacterGroupItem>{
                                                                            CharacterGroupItem{
                                                                                CharacterRange{
                                                                                    U'a',
                                                                                    U'z'
                                                                                }
                                                                            },
                                                                            CharacterGroupItem{
                                                                                CharacterRange{
                                                                                    U'0',
                                                                                    U'9'
                                                                                }
                                                                            }
                                                                        }
                                                                    }
                                                                }
                                                            },
                                                            Quantifier{
                                                                QuantifierType::OneOrMore,
                                                                std::nullopt,
                                                                std::nullopt
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }),
                                    std::nullopt
                                }
                            }
                        }
                    }
                }
            }
        },
        ""
    });

    expect_ok(parse_regex("\\d"), ParseValue<Regex>{
        Regex{
            std::nullopt,
            Expression{
                std::vector<SubExpression>{
                    SubExpression{
                        std::vector<SubExpressionItem>{
                            SubExpressionItem{
                                Match{
                                    MatchItem{
                                        MatchCharacterClass{
                                            CharacterClass{CharacterClassType::AnyDecimalDigit}
                                        }
                                    },
                                    std::nullopt
                                }
                            }
                        }
                    }
                }
            }
        }
    });
    expect_error(parse_regex("([a-z0-9]+"));
}