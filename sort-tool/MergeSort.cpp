#include "MergeSort.hpp"

namespace ksort
{
void merge(std::vector<std::string> &array, std::vector<std::string> &auxilliary_array, int start_index, int midpoint_index, int end_index)
{
    // Copy to auxilliary array
    for (int index = start_index; index <= end_index; index++)
    {
        auxilliary_array[index] = array[index];
    }
    // Merge
    int left = start_index, right = midpoint_index + 1;
    for (int index = start_index; index <= end_index; index++)
    {
        if (left > midpoint_index)
        {
            array[index] = auxilliary_array[right];
            right++;
        }
        else if (right > end_index)
        {
            array[index] = auxilliary_array[left];
            left++;
        }
        else if (auxilliary_array[right] < auxilliary_array[left])
        {
            array[index] = auxilliary_array[right];
            right++;
        }
        else
        {
            array[index] = auxilliary_array[left];
            left++;
        }
    }
}

void merge_sort(std::vector<std::string> &array, std::vector<std::string> &auxilliary_array, int start_index, int end_index)
{
    if (start_index >= end_index)
    {
        return;
    }
    int midpoint_index = start_index + (end_index - start_index) / 2;
    merge_sort(array, auxilliary_array, start_index, midpoint_index);
    merge_sort(array, auxilliary_array, midpoint_index + 1, end_index);
    merge(array, auxilliary_array, start_index, midpoint_index, end_index);
}

void merge_sort(std::vector<std::string> &array)
{
    std::vector<std::string> auxilliary_array(array.size());
    merge_sort(array, auxilliary_array, 0, array.size() - 1);
}
}   // namespace ksort