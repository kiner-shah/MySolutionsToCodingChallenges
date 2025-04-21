#include "HeapSort.hpp"

namespace ksort
{
void heapify(std::vector<std::string> &array, std::size_t index, std::size_t size)
{
    while (2 * index <= size)
    {
        std::size_t temp = 2 * index;
        if (temp < size && array[temp - 1] < array[temp])
        {
            temp++;
        }
        if (array[index - 1] >= array[temp - 1])
        {
            break;
        }
        std::swap(array[index - 1], array[temp - 1]);
        index = temp;
    }
}

void heap_sort(std::vector<std::string> &array)
{
    std::size_t length = array.size();

    // construct max heap
    for (std::size_t index = length / 2; index >= 1; index--)
    {
        heapify(array, index, length);
    }
    // sort
    std::size_t index = length;
    while (index > 1)
    {
        std::swap(array[0], array[index - 1]);
        index--;
        heapify(array, 1, index);
    }
}
}   // namespace ksort