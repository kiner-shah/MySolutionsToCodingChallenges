#include "RadixSort.hpp"

namespace ksort
{
constexpr unsigned int ASCII_RANGE = 256;

int char_at(const std::string& s, std::size_t kth_index)
{
    if (kth_index < s.length())
    {
        unsigned char uchar = s.at(kth_index);
        int c = uchar;
        return c;
    }
    return -1;
}

void radix_sort(std::vector<std::string> &array, std::vector<std::string> &auxilliary_array, std::size_t start_index, std::size_t end_index, std::size_t kth_index)
{
    std::vector<unsigned int> count(ASCII_RANGE + 1);
    
    for (auto index = start_index; index <= end_index; index++)
    {
        int c = char_at(array[index], kth_index);
        count[c + 2]++;
    }

    for (std::size_t index = 0; index < ASCII_RANGE; index++)
    {
        count[index + 1] += count[index];
    }

    for (auto index = start_index; index <= end_index; index++)
    {
        int c = char_at(array[index], kth_index);
        auxilliary_array[count[c + 1]] = array[index];
        count[c + 1]++;
    }

    for (auto index = start_index; index <= end_index; index++)
    {
        array[index] = auxilliary_array[index - start_index];
    }

    for (std::size_t index = 0; index < ASCII_RANGE; index++)
    {
        if (count[index] < count[index + 1])
        {
            radix_sort(array, auxilliary_array, start_index + count[index], start_index + count[index + 1] - 1, kth_index + 1);
        }
    }
}

void radix_sort(std::vector<std::string> &array)
{
    std::vector<std::string> auxilliary_array(array.size());
    radix_sort(array, auxilliary_array, 0, array.size() - 1, 0);
}

/*
# Bing Co-pilot suggested Python code:
def msd_radix_sort(arr, digit=0):
    if len(arr) <= 1:
        return arr

    # Create buckets for each character and one for shorter strings
    buckets = [[] for _ in range(256)]  # ASCII characters
    shorter = []

    for string in arr:
        if digit < len(string):
            buckets[ord(string[digit])].append(string)
        else:
            shorter.append(string)

    # Recursively sort each bucket
    for i in range(256):
        buckets[i] = msd_radix_sort(buckets[i], digit + 1)

    # Concatenate the results
    return shorter + [string for bucket in buckets for string in bucket]

# Example usage
arr = ["banana", "apple", "orange", "grape", "kiwi"]
sorted_arr = msd_radix_sort(arr)
print(sorted_arr)

*/

}   // namespace ksort