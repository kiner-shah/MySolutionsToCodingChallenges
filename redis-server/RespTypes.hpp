#pragma once

#include <variant>
#include <string>
#include <vector>
#include <ostream>

namespace kredis
{
struct RespString
{
    std::string m_str;
    bool m_is_bulk;

    friend std::ostream& operator<<(std::ostream& os, const RespString& resp_string);
};

using RespError = std::string;
using RespInt = std::int64_t;
using RespNull = std::nullptr_t;
using RespArray = std::vector<std::variant<RespString, RespInt, RespNull>>;
using RespType = std::variant<RespString, RespInt, RespError, RespArray, RespNull>;
}   // namespace kredis