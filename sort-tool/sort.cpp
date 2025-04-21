#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <string>
#include "QuickSort.hpp"
#include "MergeSort.hpp"
#include "HeapSort.hpp"
#include "RadixSort.hpp"
#include "RandomSort.hpp"

namespace
{
enum SortAlgorithm
{
    k_quick_sort,
    k_radix_sort,
    k_merge_sort,
    k_heap_sort,
    k_random_sort
};

std::string sort_algo_to_string(SortAlgorithm algo)
{
    switch (algo)
    {
        case SortAlgorithm::k_quick_sort: return "QUICK SORT";
        case SortAlgorithm::k_radix_sort: return "MSD RADIX SORT";
        case SortAlgorithm::k_merge_sort: return "MERGE SORT";
        case SortAlgorithm::k_heap_sort: return "HEAP SORT";
        case SortAlgorithm::k_random_sort: return "RANDOM SORT";
        default: return "UNKNOWN SORT ALGORITHM";
    }
}

struct Options
{
    bool m_is_unique = false;
    SortAlgorithm m_sort_type = SortAlgorithm::k_quick_sort;
    std::string m_file_path;

    friend std::ostream& operator<<(std::ostream& os, const Options& options)
    {
        os << "Is unique: " << std::boolalpha << options.m_is_unique << '\n';
        os << "Sort algorithm: " << sort_algo_to_string(options.m_sort_type) << '\n';
        os << "File path: " << options.m_file_path;
        return os;
    }
};

void print_usage(std::string program_name)
{
    std::cout << "Usage: " << program_name << " [OPTIONS] [FILE]\n";
    std::cout << "\n\n -u\tOutput unique values (removes duplicates)\n";
    std::cout << " -q\tUse Quick Sort. This is default.\n";
    std::cout << " -r\tUse MSD Radix Sort\n";
    std::cout << " -m\tUse Merge Sort\n";
    std::cout << " -h\tUse Heap Sort\n";
    std::cout << " -R\tUse Random Sort\n";
    std::cout << " FILE\tFile from where input strings are to be read. Expects one string per line. If absent, takes input from stdin.\n";
}

Options parse_arguments(int argc, char** argv)
{
    int count = 1;
    Options options;
    while (count < argc)
    {
        std::string arg{argv[count]};
        if (arg == "-u")
        {
            options.m_is_unique = true;
        }
        else if (arg == "-q")
        {
            options.m_sort_type = SortAlgorithm::k_quick_sort;
        }
        else if (arg == "-r")
        {
            options.m_sort_type = SortAlgorithm::k_radix_sort;
        }
        else if (arg == "-m")
        {
            options.m_sort_type = SortAlgorithm::k_merge_sort;
        }
        else if (arg == "-h")
        {
            options.m_sort_type = SortAlgorithm::k_heap_sort;
        }
        else if (arg == "-R")
        {
            options.m_sort_type = SortAlgorithm::k_random_sort;
        }
        else
        {
            options.m_file_path = arg;
        }
        count++;
    }
    return options;
}

std::vector<std::string> read_lines(std::istream& in)
{
    std::vector<std::string> lines;
    std::string line{};
    while (std::getline(in, line))
    {
        lines.emplace_back(line);
    }
    return lines;
}

std::vector<std::string> read_input(const Options& options)
{
    if (options.m_file_path.empty())
    {
        return read_lines(std::cin);
    }
    std::ifstream in(options.m_file_path);
    std::vector<std::string> lines;
    if (in.good())
    {
        lines = read_lines(in);
    }
    in.close();
    return lines;
}
}   // namespace


int main(int argc, char** argv)
{
    if (argc < 2)
    {
        print_usage(argv[0]);
        return 1;
    }
    Options options = parse_arguments(argc, argv);
    std::cerr << options << '\n';

    std::vector<std::string> lines = read_input(options);
    if (lines.empty())
    {
        std::cerr << "Error reading file: " << options.m_file_path << '\n';
        return 1;
    }

    switch (options.m_sort_type)
    {
        case SortAlgorithm::k_heap_sort:
            ksort::heap_sort(lines);
            break;
        case SortAlgorithm::k_merge_sort:
            ksort::merge_sort(lines);
            break;
        case SortAlgorithm::k_quick_sort:
            ksort::quick_sort(lines);
            break;
        case SortAlgorithm::k_radix_sort:
            ksort::radix_sort(lines);
            break;
        case SortAlgorithm::k_random_sort:
            ksort::random_sort(lines);
            break;
        default:
            std::cerr << "Error: Unknown algorithm\n";
            return 1;
    }
    if (options.m_is_unique)
    {
        std::cout << lines[0] << '\n';
        for (std::size_t index = 1; index < lines.size(); index++)
        {
            if (lines[index] != lines[index - 1])
            {
                std::cout << lines[index] << '\n';
            }
        }
    }
    else
    {
        for (const auto& line : lines)
        {
            std::cout << line << '\n';
        }
    }
}