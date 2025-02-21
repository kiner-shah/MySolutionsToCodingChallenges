#pragma once

#include "Tokenizer.hpp"

namespace kjson
{
class Parser
{
public:
    bool parse(const std::vector<Token>& tokens);
};
}   // namespace kjson