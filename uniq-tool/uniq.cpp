#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <optional>

namespace
{
struct Config
{
    std::optional<std::string> input_file_path = std::nullopt;
    std::optional<std::string> output_file_path = std::nullopt;
    bool print_count = false;
    bool print_only_repeated = false;
    bool print_only_unique = false;

    friend std::ostream& operator<<(std::ostream& os, const Config& config)
    {
        os << "Config:\nInput file path: " << (config.input_file_path ? *config.input_file_path : "STDIN")
           << "\nOutput file path: " << (config.output_file_path ? *config.output_file_path : "STDOUT")
           << "\nPrint count: " << config.print_count
           << "\nPrint only repeated: " << config.print_only_repeated
           << "\nPrint only unique: " << config.print_only_unique;
        return os;
    }
};

void print_usage(const char* program_name)
{
    std::cerr << "Usage: " << program_name << " [OPTIONS] [INPUT_FILE [OUTPUT_FILE]]\n"
              << "\nOPTIONS:\n"
              << "  -c, --count       Prefix lines by the number of occurrences\n"
              << "  -d, --repeated    Only print duplicate (repeated) lines\n"
              << "  -u, --unique      Only print unique lines\n";
}

void parse_arguments(int argc, char** argv, Config& config)
{
    bool input_stdin = false;
    for (int i = 1; i < argc; i++)
    {
        std::string_view arg{argv[i]};
        if (arg == "-c" || arg == "--count")
        {
            config.print_count = true;
        }
        else if (arg == "-d" || arg == "--repeated")
        {
            if (config.print_only_unique)
            {
                std::cerr << "Cannot specify both --repeated and --unique options.\n";
                std::exit(EXIT_FAILURE);
            }
            else
            {
                config.print_only_repeated = true;
            }
        }
        else if (arg == "-u" || arg == "--unique")
        {
            if (config.print_only_repeated)
            {
                std::cerr << "Cannot specify both --repeated and --unique options.\n";
                std::exit(EXIT_FAILURE);
            }
            else
            {
                config.print_only_unique = true;
            }
        }
        else if (arg == "-")
        {
            input_stdin = true;
        }
        else if (arg.starts_with("-"))
        {
            std::cerr << "Unknown option: " << arg << '\n';
            print_usage(argv[0]);
            std::exit(EXIT_FAILURE);
        }
        else
        {
            if (!input_stdin && !config.input_file_path)
            {
                config.input_file_path = arg;
            }
            else if (!config.output_file_path)
            {
                config.output_file_path = arg;
            }
            else
            {
                std::cerr << "Too many positional arguments.\n";
                print_usage(argv[0]);
                std::exit(EXIT_FAILURE);
            }
        }
    }
}

void print_line(std::ostream& output, const std::string& line, int count, bool print_count, bool print_only_repeated, bool print_only_unique)
{
    auto print = [&count, &line, &output, &print_count]()
    {
        if (print_count)
        {
            output << std::setw(7) << count << ' ';
        }
        output << line << '\n';
    };

    if (print_only_repeated && count > 1)
    {
        print();
    }
    else if (print_only_unique && count == 1)
    {
        print();
    }
    else if (!print_only_repeated && !print_only_unique)
    {
        print();
    }
}

void process_lines(std::istream& input, std::ostream& output, bool print_count, bool print_only_repeated, bool print_only_unique)
{
    std::string current_line{};
    std::string previous_line{};

    if (!std::getline(input, current_line))
    {
        std::cerr << "Input is empty.\n";
        return;
    }
    previous_line = current_line;

    int count = 1;

    while (std::getline(input, current_line))
    {
        if (current_line == previous_line)
        {
            count++;
        }
        else
        {
            print_line(output, previous_line, count, print_count, print_only_repeated, print_only_unique);
            count = 1;
        }
        previous_line = current_line;
    }
    print_line(output, previous_line, count, print_count, print_only_repeated, print_only_unique);
}

void process(const Config& config)
{
    std::ifstream input_file;
    std::istream& input = (config.input_file_path ? input_file : std::cin);

    std::ofstream output_file;
    std::ostream& output = (config.output_file_path ? output_file : std::cout);
    if (config.input_file_path)
    {
        input_file.open(*config.input_file_path);
        if (!input_file)
        {
            std::cerr << "Failed to open input file: " << *config.input_file_path << '\n';
            std::exit(EXIT_FAILURE);
        }
    }
    if (config.output_file_path)
    {
        output_file.open(*config.output_file_path);
        if (!output_file)
        {
            std::cerr << "Failed to open output file: " << *config.output_file_path << '\n';
            std::exit(EXIT_FAILURE);
        }
    }
    process_lines(input, output, config.print_count, config.print_only_repeated, config.print_only_unique);
}
}   // namespace

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }
    Config config;
    parse_arguments(argc, argv, config);

    // std::cout << config << '\n';
    process(config);
}
