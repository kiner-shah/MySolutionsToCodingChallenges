#pragma once

#include <variant>
#include <string>
#include <vector>
#include <ostream>
#include <memory>

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

// Forward declaration for recursive types
struct RespType;

// Now RespArray can contain any RespType, enabling full nesting
using RespArray = std::vector<RespType>;

// Define the main variant type that includes all RESP types
struct RespType : std::variant<RespString, RespInt, RespError, RespArray, RespNull>
{
    using variant::variant;
};

using RespTypePtr = std::shared_ptr<RespType>;
}   // namespace kredis