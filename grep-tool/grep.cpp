#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include "parser.hpp"
#include "nfa.hpp"
#include "matcher.hpp"

namespace
{
bool convert_to_u32string(std::string_view input, std::u32string& output)
{
    std::size_t i = 0;
    while (i < input.size())
    {
        // UTF-8 decoding algorithm
        char32_t utf8_code_point = 0;
        unsigned char c = input[i];
        if (c <= 0x7F)
        {
            utf8_code_point = c;
            ++i;
        }
        else if (c >= 0xC0 && c <= 0xDF)
        {
            if (i + 1 >= input.size())
            {
                // std::cerr << "Invalid UTF-8 byte sequence for codepoint range U+0080 - U+07FF\n";
                return false;
            }
            unsigned char c1 = input[i + 1];
            if (c1 >= 0x80 && c1 <= 0xBF)
            {
                utf8_code_point = ((c & 0x1F) << 6) | (c1 & 0x3F);
                i += 2;
            }
            else
            {
                // std::cerr << "Invalid UTF-8 byte sequence for codepoint range U+0080 - U+07FF\n";
                return false;
            }
        }
        else if (c >= 0xE0 && c <= 0xEF)
        {
            if (i + 1 >= input.size() || i + 2 >= input.size())
            {
                // std::cerr << "Invalid UTF-8 byte sequence for codepoint range U+0800 - U+FFFF\n";
                return false;
            }
            unsigned char c1 = input[i + 1];
            unsigned char c2 = input[i + 2];
            if (c1 >= 0x80 && c1 <= 0xBF && c2 >= 0x80 && c2 <= 0xBF)
            {
                utf8_code_point = ((c & 0xF) << 12) | ((c1 & 0x3F) << 6) | (c2 & 0x3F);
                i += 3;
            }
            else
            {
                // std::cerr << "Invalid UTF-8 byte sequence for codepoint range U+0800 - U+FFFF\n";
                return false;
            }
        }
        else if (c >= 0xF0 && c <= 0xF7)
        {
            if (i + 1 >= input.size() || i + 2 >= input.size() || i + 3 >= input.size())
            {
                // std::cerr << "Invalid UTF-8 byte sequence for codepoint range U+10000 - U+10FFFF\n";
                return false;
            }
            unsigned char c1 = input[i + 1];
            unsigned char c2 = input[i + 2];
            unsigned char c3 = input[i + 3];
            if (c1 >= 0x80 && c1 <= 0xBF && c2 >= 0x80 && c2 <= 0xBF && c3 >= 0x80 && c3 <= 0xBF)
            {
                utf8_code_point = ((c & 0x7) << 18) | ((c1 & 0x3F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
                i += 4;
            }
            else
            {
                // std::cerr << "Invalid UTF-8 byte sequence for codepoint range U+10000 - U+10FFFF\n";
                return false;
            }
        }
        else
        {
            // std::cerr << "Invalid UTF-8 byte sequence\n";
            return false;
        }
        output.push_back(utf8_code_point);
    }
    return true;
}

void print_usage(const char* program_name)
{
    std::cerr << "Usage: " << program_name << " [options] <pattern> <file>\n";
    std::cerr << "\nOptions:\n";
    std::cerr << "  -i, --ignore-case    Perform case-insensitive matching\n";
    std::cerr << "  -v, --invert-match   Invert the sense of matching, to select non-matching lines\n";
    std::cerr << "  -r, --recursive      Read all files under each directory, recursively\n";
}

struct File
{
    // If path is not set, it means we should from stdin
    std::optional<std::string> path = std::nullopt;
};

struct Config
{
    bool ignore_case = false;
    bool recursive = false;
    bool invert_match = false;
    std::string pattern{};
    // If file is empty, it means we should read from stdin
    std::vector<File> file = {};

    friend std::ostream& operator<<(std::ostream& os, const Config& config)
    {
        os << "Config\n";
        os << "  ignore_case: " << config.ignore_case << '\n';
        os << "  recursive: " << config.recursive << '\n';
        os << "  invert_match: " << config.invert_match << '\n';
        os << "  pattern: " << config.pattern << '\n';
        if (!config.file.empty())
        {
            for (const auto& file : config.file)
            {
                if (file.path.has_value())
                {
                    os << "  file: " << file.path.value() << '\n';
                }
                else
                {
                    os << "  file: <stdin>\n";
                }
            }
        }
        else
        {
            os << "  file: <stdin>\n";
        }
        return os;
    }
};

int try_parse_options(int argc, char** argv, Config& config)
{
    int index = 1;
    while (index < argc)
    {
        std::string arg{argv[index]};
        if (arg == "-i" || arg == "--ignore-case")
        {
            config.ignore_case = true;
        }
        else if (arg == "-v" || arg == "--invert-match")
        {
            config.invert_match = true;
        }
        else if (arg == "-r" || arg == "--recursive")
        {
            config.recursive = true;
        }
        else
        {
            break;
        }
        index++;
    }
    return index;
}

bool parse_cmdline_args(int argc, char** argv, Config& config)
{
    int index = try_parse_options(argc, argv, config);
    if (index >= argc)
    {
        print_usage(argv[0]);
        return false;
    }
    config.pattern = std::string{argv[index]};
    if (index + 1 < argc)
    {
        int j = index + 1;
        config.file = std::vector<File>{};
        while (j < argc)
        {
            std::string arg{argv[j]};
            if (arg != "-")
            {
                config.file.emplace_back(File{arg});
            }
            else
            {
                config.file.emplace_back(File{std::nullopt});
            }
            j++;
        }
    }
    return true;
}

using RegexMatcher = std::optional<kregex::Matcher>;

RegexMatcher compile_empty_regex()
{
    kregex::NfaElement nfa_element{0, 0};
    std::vector<kregex::State> nfa_states(1);
    nfa_states[0].id = 0;
    nfa_states[0].transitions = {};
    return std::make_optional(kregex::Matcher{nfa_element, nfa_states});
}

RegexMatcher compile_regex(std::string_view pattern)
{
    if (pattern.empty())
    {
        return compile_empty_regex();
    }
    auto parse_result = kregex::parse_regex(pattern);
    if (std::holds_alternative<kregex::ParseError>(parse_result))
    {
        std::cerr << "Failed to parse regex: " << std::get<kregex::ParseError>(parse_result) << '\n';
        return std::nullopt;
    }
    auto regex = std::get<kregex::ParseValue<kregex::Regex>>(parse_result).value;
    kregex::Nfa nfa;
    auto nfa_element = nfa.build_regex(regex);
    auto nfa_states = nfa.get_states();
    return std::make_optional(kregex::Matcher{nfa_element, nfa_states});
}

std::optional<std::vector<kregex::MatchResult>> regex_match(
    const RegexMatcher& matcher,
    std::u32string_view line)
{
    if (!matcher.has_value())
    {
        return std::nullopt;
    }
    return matcher->match_all(line);
}

bool is_binary_file(std::istream& input)
{
    constexpr std::size_t sample_size = 512;
    std::array<char, sample_size> buffer{};
    input.read(buffer.data(), buffer.size());
    const auto bytes_read = input.gcount();
    for (std::size_t i = 0; i < static_cast<std::size_t>(bytes_read); ++i)
    {
        unsigned char c = buffer[i];
        if (c == '\0'
            || (c < 31 && c != '\n' && c != '\r' && c != '\t'))
        {
            return true;
        }
    }
    input.clear();
    input.seekg(0);
    return false;
}

bool process_one_file(
    std::istream& input,
    const RegexMatcher& matcher,
    bool is_inverted = false,
    bool ignore_case = false,
    std::optional<std::filesystem::path> file_path = std::nullopt)
{
    bool any_match = false;
    std::string line;
    std::u32string u32_line{};
    
    while (std::getline(input, line))
    {
        std::string original_line = line;
        if (ignore_case)
        {
            std::transform(
                original_line.cbegin(),
                original_line.cend(),
                line.begin(),
                [](unsigned char c) { return std::tolower(c); });
        }

        u32_line.clear();
        u32_line.reserve(line.size());
        if (!convert_to_u32string(line, u32_line))
        {
            continue;
        }
        auto match_results = regex_match(matcher, u32_line);
        if (!is_inverted)
        {
            if (match_results.has_value() && !match_results->empty())
            {
                if (file_path.has_value())
                {
                    std::cout << file_path.value() << ':';
                }
                std::cout << original_line << '\n';
                any_match = true;
            }
        }
        else
        {
            if (!match_results.has_value() || match_results->empty())
            {
                if (file_path.has_value())
                {
                    std::cout << file_path.value().generic_string() << ':';
                }
                std::cout << original_line << '\n';
                any_match = true;
            }
        }
    }
    return any_match;
}

bool process_recursive_search(
    std::filesystem::path directory,
    const RegexMatcher& matcher,
    bool is_inverted = false,
    bool ignore_case = false)
{
    for (const auto& dir_entry : std::filesystem::recursive_directory_iterator(directory))
    {
        if (dir_entry.is_regular_file())
        {
            std::ifstream input_file{dir_entry.path()};
            if (!input_file)
            {
                std::cerr << "Failed to open file: " << dir_entry.path() << '\n';
                return false;
            }
            if (is_binary_file(input_file))
            {
                input_file.close();
                continue;
            }
            auto result = process_one_file(input_file, matcher, is_inverted, ignore_case, dir_entry.path());
            input_file.close();
            if (!result)
            {
                continue;
            }
        }
    }
    return true;
}
}   // namespace

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }
    Config config{};
    if (!parse_cmdline_args(argc, argv, config))
    {
        return EXIT_FAILURE;
    }
    // std::cout << config << '\n';

    auto process_valid_file = [](
        const std::filesystem::path& path,
        const RegexMatcher& matcher,
        bool is_inverted = false,
        bool ignore_case = false,
        bool print_path = false)
    {
        if (!std::filesystem::is_regular_file(path))
        {
            std::cerr << "Not a regular file: " << path << '\n';
            return false;
        }

        std::ifstream input_file{path};
        if (!input_file)
        {
            std::cerr << "Failed to open file: " << path << '\n';
            return false;
        }
        if (is_binary_file(input_file))
        {
            input_file.close();
            return true;
        }

        if (!process_one_file(input_file, matcher, is_inverted, ignore_case, print_path ? std::make_optional(path) : std::nullopt))
        {
            input_file.close();
            return false;
        }
        input_file.close();
        return true;
    };

    auto process_stdin = [&config](const RegexMatcher& matcher)
    {
        return process_one_file(std::cin, matcher, config.invert_match, config.ignore_case);
    };
    
    if (config.ignore_case)
    {
        std::transform(
            config.pattern.cbegin(),
            config.pattern.cend(),
            config.pattern.begin(),
            [](unsigned char c) { return std::tolower(c); });
    }

    RegexMatcher regex_compile_result = std::nullopt;
    regex_compile_result = compile_regex(config.pattern);
    if (!regex_compile_result.has_value())
    {
        return EXIT_FAILURE;
    }

    if (config.recursive)
    {
        if (!config.file.empty())
        {

            bool any_match = false;
            for (const auto& file : config.file)
            {
                if (file.path.has_value())
                {
                    std::filesystem::path path{file.path.value()};
                    if (std::filesystem::is_directory(path))
                    {
                        if (!process_recursive_search(path, regex_compile_result, config.invert_match, config.ignore_case))
                        {
                            continue;
                        }
                        any_match = true;
                    }
                    else
                    {
                        if (!process_valid_file(path, regex_compile_result, config.invert_match, config.ignore_case, true))
                        {
                            continue;
                        }
                        any_match = true;
                    }
                }
                else
                {
                    if (!process_stdin(regex_compile_result))
                    {
                        continue;
                    }
                    any_match = true;
                }
            }
            if (!any_match)
            {
                return EXIT_FAILURE;
            }
        }
        else
        {
            if (!process_recursive_search(std::filesystem::current_path(), regex_compile_result, config.invert_match, config.ignore_case))
            {
                return EXIT_FAILURE;
            }
        }
    }
    else if (!config.file.empty())
    {
        bool print_path = config.file.size() > 1;
        bool any_match = false;
        for (const auto& file : config.file)
        {
            if (file.path.has_value())
            {
                std::filesystem::path path{file.path.value()};
                if (!process_valid_file(path, regex_compile_result, config.invert_match, config.ignore_case, print_path))
                {
                    continue;
                }
                any_match = true;
            }
            else
            {
                if (!process_stdin(regex_compile_result))
                {
                    continue;
                }
                any_match = true;
            }
        }
        if (!any_match)
        {
            return EXIT_FAILURE;
        }
    }
    else
    {
        if (!process_stdin(regex_compile_result))
        {
            return EXIT_FAILURE;
        }
    }
}