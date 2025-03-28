#include <iostream>
#include <fstream>
#include <vector>
#include <charconv>

namespace
{
struct Config
{
    std::vector<uint_least64_t> m_fields;
    std::string_view m_input_file;
    char m_delimiter = '\t';    // Default delimiter is TAB.

    friend std::ostream& operator<<(std::ostream& os, const Config& config)
    {
        os << "Input file: " << config.m_input_file << '\n';
        os << "Delimiter: " << config.m_delimiter << "(0x" << std::hex << static_cast<int>(config.m_delimiter) << std::dec << ")\n";
        os << "Fields: ";
        for (unsigned int field : config.m_fields)
        {
            os << field << ' ';
        }
        return os;
    }
};

void print_usage(std::string program_name)
{
    std::cout << "Usage: " << program_name << " OPTION... FILE\n\n";
    std::cout << "  -d DELIMITER\tUse delimiter instead of TAB for field delimiter\n";
    std::cout << "  -f FIELD_LIST\tSelect only these fields. Field list should contain numbers in range [1...].\n";
    std::cout << "               \tIn case no delimiters are present, print the whole line.\n";
}

constexpr bool starts_with(std::string_view source, std::string_view target)
{
    return source.find(target) == 0;
}

constexpr bool starts_with(std::string_view source, char target)
{
    return source.find(target) == 0;
}

constexpr bool ends_with(std::string_view source, char target)
{
    return source.rfind(target) == source.size() - 1;
}

bool convert_to_int64(const char* start, const char* end, std::uint_least64_t& value)
{
    auto [ptr, ec] = std::from_chars(start, end, value);
    return ptr == end && ec != std::errc::invalid_argument && ec != std::errc::result_out_of_range;
}

bool split_by_delimiter_and_extract_fields(std::string_view field, Config& config, char delimiter = ' ')
{
    bool no_error = true;
    while (true)
    {
        auto pos = field.find(delimiter);
        std::uint_least64_t value = 0;
        if (pos == std::string_view::npos)
        {
            if (!convert_to_int64(field.data(), field.data() + field.size(), value))
            {
                config.m_fields.clear();
                no_error = false;
                break;
            }
            config.m_fields.push_back(value);
            break;
        }
        else
        {
            if (!convert_to_int64(field.data(), field.data() + pos, value))
            {
                config.m_fields.clear();
                no_error = false;
                break;
            }
            field = field.substr(pos + 1);
            config.m_fields.push_back(value);
        }
    }
    return no_error;
}

constexpr std::string_view remove_quotes(std::string_view field)
{
    if ((starts_with(field, '"') && ends_with(field, '"'))
        || (starts_with(field, '\'') && ends_with(field, '\'')))
    {
        field = field.substr(1, field.size() - 2);
    }
    return field;
}

constexpr bool extract_fields(std::string_view field, Config& config)
{
    field = remove_quotes(field);
    if (field.empty())
    {
        return false;
    }

    if (field.find(',') != std::string_view::npos)
    {
        return split_by_delimiter_and_extract_fields(field, config, ',');
    }
    else
    {
        return split_by_delimiter_and_extract_fields(field, config);
    }
}

void read_lines(std::istream& input, std::vector<std::string>& lines)
{
    std::string line;
    while (std::getline(input, line))
    {
        lines.push_back(line);
    }
}

void cut_and_print_output(const Config& config, const std::vector<std::string>& lines)
{
    for (const std::string& line : lines)
    {
        std::string_view line_view{line};
        std::vector<std::string_view> field_values;
        while (true)
        {
            auto pos = line_view.find(config.m_delimiter);
            if (pos == std::string_view::npos)
            {
                field_values.push_back(line_view);
                break;
            }
            else
            {
                field_values.push_back(line_view.substr(0, pos));
                line_view = line_view.substr(pos + 1);
            }
        }
        bool first_field = true;
        for (const auto field_no : config.m_fields)
        {
            if (field_no - 1 < field_values.size())
            {
                if (!first_field)
                {
                    std::cout << config.m_delimiter;
                }
                std::cout << field_values.at(field_no - 1);
                if (first_field)
                {
                    first_field = false;
                }
            }
        }
        std::cout << '\n';
    }
}
}   // namespace

int main(int argc, char** argv)
{
    if (argc <= 2)
    {
        print_usage(argv[0]);
        return 1;
    }

    Config config;
    int arg_index = 1;
    while (arg_index < argc)
    {
        std::string_view arg{argv[arg_index]};
        if (arg == "-f")
        {
            // Extract next arg string and parse for fields
            ++arg_index;
            std::string_view field{argv[arg_index]};
            if (!extract_fields(field, config))
            {
                std::cerr << "Some error occured during parsing fields\n";
                return 1;
            }
        }
        else if (starts_with(arg, "-f"))
        {
            // Extract substring [2...] and parse for fields
            std::string_view field{argv[arg_index]};
            field = field.substr(2);
            if (!extract_fields(field, config))
            {
                std::cerr << "Some error occured during parsing fields\n";
                return 1;
            }
        }
        else if (arg == "-d")
        {
            // Next arg is a delimiter
            ++arg_index;
            std::string_view delimiter{argv[arg_index]};
            delimiter = remove_quotes(delimiter);
            if (delimiter.size() > 1)
            {
                std::cerr << "Delimiter should be 1 character only\n";
                return 1;
            }
            if (!delimiter.empty())
            {
                config.m_delimiter = delimiter[0];
            }
        }
        else if (starts_with(arg, "-d"))
        {
            // Extract substring [2...] and parse for delimiter
            std::string_view delimiter{argv[arg_index]};
            delimiter = delimiter.substr(2);
            delimiter = remove_quotes(delimiter);
            if (delimiter.size() > 1)
            {
                std::cerr << "Delimiter should be 1 character only\n";
                return 1;
            }
            if (!delimiter.empty())
            {
                config.m_delimiter = delimiter[0];
            }
        }
        else
        {
            config.m_input_file = std::string_view{argv[arg_index]};
        }
        ++arg_index;
    }
    if (config.m_fields.empty())
    {
        std::cerr << "One or more fields are required\n";
        return 1;
    }
    //std::cout << config << '\n';

    std::vector<std::string> lines;
    if (config.m_input_file.empty() || config.m_input_file == "-")
    {
        // Handle read from stdin
        read_lines(std::cin, lines);
    }
    else
    {
        // Handle read from file
        std::ifstream input_file(config.m_input_file.data(), std::ios::binary);
        if (!input_file.good())
        {
            std::cerr << "Cannot open input file " << config.m_input_file << '\n';
            return 1;
        }
        read_lines(input_file, lines);
        input_file.close();
    }
    cut_and_print_output(config, lines);
}