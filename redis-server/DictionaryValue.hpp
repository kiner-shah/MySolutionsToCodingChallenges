#pragma once

#include "RespTypes.hpp"
#include <optional>

namespace kredis
{
struct DictionaryValue
{
    RespTypePtr m_value;
    std::optional<std::uint64_t> m_expiry_timestamp;
};
}   // namespace kredis