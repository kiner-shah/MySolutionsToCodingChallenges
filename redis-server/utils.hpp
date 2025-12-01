#pragma once
#include <chrono>
#include <string>
#include <optional>

namespace kredis
{
template <typename DurationType = std::chrono::seconds>
std::uint64_t get_current_time()
{
    auto timepoint = std::chrono::high_resolution_clock::now();
    auto value = std::chrono::duration_cast<DurationType>(timepoint.time_since_epoch());
    return value.count();
}

std::optional<std::int64_t> is_valid_signed_64_bit_int(const std::string& value);
}   // namespace kredis