#include "utils.hpp"
#include <charconv>

namespace kredis
{
std::optional<std::int64_t> is_valid_signed_64_bit_int(const std::string &value)
{
    std::int64_t integer_value;
    const char* start_ptr = value.data();
    const char* end_ptr = value.data() + value.size();

    auto result = std::from_chars(start_ptr, end_ptr, integer_value);
    if (result.ec == std::errc{} && result.ptr == end_ptr)
    {
        return integer_value;
    }
    return std::nullopt;
}
}   // namespace kredis