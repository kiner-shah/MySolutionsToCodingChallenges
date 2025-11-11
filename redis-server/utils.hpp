#pragma once
#include <chrono>

namespace kredis
{
template <typename DurationType = std::chrono::seconds>
std::uint64_t get_current_time()
{
    auto timepoint = std::chrono::high_resolution_clock::now();
    auto value = std::chrono::duration_cast<DurationType>(timepoint.time_since_epoch());
    return value.count();
}
}   // namespace kredis