#include "QuickSort.hpp"

namespace ksort
{
int partition(std::vector<std::string> &array, int start_index, int end_index)
{
    int left = start_index, right = end_index + 1;
    while (true)
    {
        while (array[++left] < array[start_index])
        {
            if (left == end_index)
            {
                break;
            }
        }
        while (array[start_index] < array[--right])
        {
            if (right == start_index)
            {
                break;
            }
        }
        if (left >= right)
        {
            break;
        }
        std::swap(array[left], array[right]);
    }
    std::swap(array[start_index], array[right]);
    return right;
}

void quick_sort(std::vector<std::string> &array, int start_index, int end_index)
{
    if (start_index >= end_index)
    {
        return;
    }
    int partition_index = partition(array, start_index, end_index);
    quick_sort(array, start_index, partition_index - 1);
    quick_sort(array, partition_index + 1, end_index);
}

void quick_sort_3_way(std::vector<std::string> &array, int start_index, int end_index)
{
    if (start_index >= end_index)
    {
        return;
    }
    // 3-way partitioning - can be used when duplicates are present
    int left = start_index, right = end_index;
    std::string& start_element = array[start_index];
    int index = start_index;
    while (index <= right)
    {
        int compare_result = array[index].compare(start_element);
        if (compare_result < 0)
        {
            std::swap(array[left], array[index]);
            left++;
            index++;
        }
        else if (compare_result > 0)
        {
            std::swap(array[index], array[right]);
            right--;
        }
        else
        {
            index++;
        }
    }

    quick_sort(array, start_index, left - 1);
    quick_sort(array, right + 1, end_index);
}

void quick_sort(std::vector<std::string> &array)
{
    quick_sort(array, 0, array.size() - 1);
    // quick_sort_3_way(array, 0, array.size() - 1);
}

}   // namespace ksort