#include "RandomSort.hpp"
#include <algorithm>
#include <random>

namespace ksort
{
// Similar to boost::hash_combine
std::size_t hash_combine(std::random_device::result_type seed, const std::string& input)
{
    std::hash<std::string> hasher;
    seed ^= hasher(input) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    return seed;
}

void random_sort(std::vector<std::string> &array)
{
    // Thanks to Reddit user u/IyeOnline
    // Reference: https://www.reddit.com/r/cpp_questions/comments/1k52c6n/comment/moen888
    std::random_device device;
    auto seed = device();
    auto seeded_hash = [&seed](const std::string& s) { return hash_combine(seed, s); };

    std::sort(array.begin(), array.end(),
        [&seeded_hash](const std::string& a, const std::string& b)
        {
            return seeded_hash(a) < seeded_hash(b);
        });
}
}   // namespace ksort