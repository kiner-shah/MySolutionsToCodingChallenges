#include <iostream>
#include <fstream>
#include <filesystem>
#include <algorithm>

namespace
{
struct Config
{
    std::string m_input_file_path = "";
    bool m_byte_counter = false;
    bool m_line_counter = false;
    bool m_word_counter = false;
    bool m_char_counter = false;

    void print()
    {
        std::cout << "input_file: " << m_input_file_path << '\n';
        std::cout << "byte_counter: " << m_byte_counter << '\n';
        std::cout << "line_counter: " << m_line_counter << '\n';
        std::cout << "word_counter: " << m_word_counter << '\n';
        std::cout << "char_counter: " << m_char_counter << '\n';
    }
};

class UTF8Decoder
{
    std::string_view m_buffer;
    unsigned int m_buffer_index;

    bool get_next_character(unsigned char& c)
    {
        if (m_buffer_index >= m_buffer.size())
        {
            return false;
        }
        c = m_buffer[m_buffer_index];
        m_buffer_index++;
        return true;
    }
public:
    static constexpr char32_t utf_replacement_character = 0xFFFD;

    UTF8Decoder(std::string_view input_string)
        : m_buffer{std::move(input_string)}, m_buffer_index{0}
    {
    }

    bool get_next_codepoint(char32_t& utf32_codepoint)
    {
        unsigned int state = 0;
        do
        {
            unsigned char c;
            if (!get_next_character(c))
            {
                return false;
            }
            // Decoding UTF-8 bytes into UTF-32 codepoints
            // Reference: https://writings.sh/post/en/utf8
            switch (state)
            {
                case 0:
                    if (c >= 0x0 && c <= 0x7f)
                    {
                        utf32_codepoint = c;
                    }
                    else if (c >= 0xc2 && c <= 0xdf)
                    {
                        state = 1;
                        utf32_codepoint = c & 0x1f;
                    }
                    else if (c == 0xe0)
                    {
                        state = 4;
                        utf32_codepoint = c & 0xf;
                    }
                    else if (c >= 0xe1 && c <= 0xef)
                    {
                        state = 2;
                        utf32_codepoint = c & 0xf;
                    }
                    else if (c == 0xf0)
                    {
                        state = 5;
                        utf32_codepoint = c & 0x7;
                    }
                    else if (c >= 0xf1 && c <= 0xf3)
                    {
                        state = 3;
                        utf32_codepoint = c & 0x7;
                    }
                    else if (c == 0xf4)
                    {
                        state = 6;
                        utf32_codepoint = c & 0x7;
                    }
                    else
                    {
                        state = 8;
                    }
                    break;
                case 1:
                case 2:
                case 3:
                    if (c >= 0x80 && c <= 0xbf)
                    {
                        state--;
                        utf32_codepoint = (utf32_codepoint << 6) | (c & 0x3f);
                    }
                    else
                    {
                        state = 8;
                    }
                    break;
                case 4:
                    if (c >= 0xa0 && c <= 0xbf)
                    {
                        state = 1;
                        utf32_codepoint = (utf32_codepoint << 6) | (c & 0x3f);
                    }
                    else
                    {
                        state = 8;
                    }
                    break;
                case 5:
                    if (c >= 0x90 && c <= 0xbf)
                    {
                        state = 2;
                        utf32_codepoint = (utf32_codepoint << 6) | (c & 0x3f);
                    }
                    else
                    {
                        state = 8;
                    }
                    break;
                case 6:
                    if (c >= 0x80 && c <= 0x8f)
                    {
                        state = 2;
                        utf32_codepoint = (utf32_codepoint << 6) | (c & 0x3f);
                    }
                    else
                    {
                        state = 8;
                    }
                    break;
                default:
                    state = 8;
                    break;
            }
        } while (state != 0 && state != 8);
        utf32_codepoint = (state == 8 ? utf_replacement_character : utf32_codepoint);
        return true;
    }
};

struct Output
{
    uint64_t m_bytes = 0;
    uint64_t m_lines = 0;
    uint64_t m_words = 0;
    uint64_t m_chars = 0;
};

void print_usage(const char* program_name)
{
    std::cerr << "Usage: " << program_name << " [OPTION]... [FILE]\n";
    std::cerr << "\nOptions:\n";
    std::cerr << "  -c\tCount bytes\n";
    std::cerr << "  -l\tCount lines\n";
    std::cerr << "  -w\tCount words\n";
    std::cerr << "  -m\tCount characters\n";
    std::cerr << "\nIf FILE is not provided, standard input is used.\n";
}

std::string read_file(const Config& config)
{
    if (!config.m_input_file_path.empty() 
        && !(std::filesystem::exists(std::filesystem::path{config.m_input_file_path})
            || std::filesystem::is_regular_file(config.m_input_file_path)))
    {
        std::cerr << "Either file doesn't exist or is not a regular file: " << config.m_input_file_path << '\n';
        return std::string{};
    }
    std::string file_contents;
    if (config.m_input_file_path.empty())
    {
        char c;
        while (std::cin.get(c))
        {
            file_contents += c;
        }
    }
    else
    {
        std::size_t file_size = std::filesystem::file_size(config.m_input_file_path);
        file_contents.resize(file_size);
        std::ifstream input(config.m_input_file_path, std::ios::binary);
        if (!input.good())
        {
            std::cerr << "Couldn't open file for reading: " << config.m_input_file_path << '\n';
            return std::string{};
        }
        if (!input.read(file_contents.data(), file_size))
        {
            std::cerr << "Failure while reading file " << config.m_input_file_path << '\n';
            input.close();
            return std::string{};
        }
        input.close();
    }
    return file_contents;
}

bool is_white_space(char32_t c)
{
    // Reference: https://en.wikipedia.org/wiki/Unicode_character_property#Whitespace
    std::array<char32_t, 25> possible_whites_spaces = {
        9,
        10,
        11,
        12,
        13,
        32,
        133,
        160,
        5760,
        8192,
        8193,
        8194,
        8195,
        8196,
        8197,
        8198,
        8199,
        8200,
        8201,
        8202,
        8232,
        8233,
        8239,
        8287,
        12288
    };
    return std::find(possible_whites_spaces.begin(), possible_whites_spaces.end(), c) != possible_whites_spaces.end();
    // return c == U' ' || c == U'\n' || c == U'\t' || c == U'\r' || c == U'\v' || c == U'\f';
}

void process_file(const std::string& file_contents, Output& output)
{
    char32_t utf32_codepoint = 0x0;
    bool is_word_under_process = false;

    output.m_bytes = file_contents.length();
    output.m_lines = std::count_if(file_contents.begin(), file_contents.end(), [](unsigned char c) { return c == '\n'; });

    UTF8Decoder decoder{file_contents};
    while (decoder.get_next_codepoint(utf32_codepoint))
    {
        output.m_chars++;
        // Process words
        if (is_white_space(utf32_codepoint) && is_word_under_process)
        {
            output.m_words++;
            is_word_under_process = false;
        }
        else if (!is_white_space(utf32_codepoint))
        {
            is_word_under_process = true;
        }
    }
}
}   // namespace

int main(int argc, char** argv)
{
    Config config;
    Output output;

    int index = 1;
    while (index < argc)
    {
        std::string arg{argv[index]};
        if (arg == "-c")
        {
            config.m_byte_counter = true;
        }
        else if (arg == "-l")
        {
            config.m_line_counter = true;
        }
        else if (arg == "-w")
        {
            config.m_word_counter = true;
        }
        else if (arg == "-m")
        {
            config.m_char_counter = true;
        }
        else if (arg == "--help")
        {
            print_usage(argv[0]);
            return 1;
        }
        else
        {
            config.m_input_file_path = arg;
        }
        index++;
    }

    if (!config.m_byte_counter && !config.m_line_counter && !config.m_word_counter && !config.m_char_counter)
    {
        config.m_byte_counter = true;
        config.m_line_counter = true;
        config.m_word_counter = true;
    }
    // config.print();

    auto file_contents = read_file(config);
    if (file_contents.empty())
    {
        return 1;
    }
    process_file(file_contents, output);

    if (config.m_line_counter)
    {
        std::cout << output.m_lines << ' ';
    }
    if (config.m_word_counter)
    {
        std::cout << output.m_words << ' ';
    }
    if (config.m_byte_counter)
    {
        std::cout << output.m_bytes << ' ';
    }
    if (config.m_char_counter)
    {
        std::cout << output.m_chars << ' ';
    }
    std::cout << '\n';
}
