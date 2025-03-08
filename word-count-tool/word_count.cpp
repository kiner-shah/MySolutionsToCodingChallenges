#include <filesystem>
#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <iomanip>
#include <array>
#include <cwctype>

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

struct Output
{
    uint64_t m_bytes = 0;
    uint64_t m_lines = 0;
    uint64_t m_words = 0;
    uint64_t m_chars = 0;
};

bool process_file(std::istream &is, Output &output)
{
    // const size_t buffer_size = MB_CUR_MAX;   // Keep this commented, not sure where this MB_CUR_MAX can be used
    constexpr size_t BUF_SIZE = 1024;
    std::array<char, BUF_SIZE> file_buf{};
    size_t remaining_bytes = 0;
    wchar_t current_wide_char, previous_wide_char = L'\0';
    bool first_char = false, is_word_under_process = false;
 
    // Sometimes std::istream::read fails after certain offset, so check also if
    // std::istream::gcount returns non-zero.
    // Reference: https://stackoverflow.com/a/22986486
    while (is.read(file_buf.data() + remaining_bytes, BUF_SIZE - remaining_bytes) || is.gcount() != 0)
    {
        // Remember to add remaining bytes to read bytes, else you will see incorrect results
        size_t bytes_read = is.gcount() + remaining_bytes;
        char* file_buf_data_ptr = file_buf.data();
        size_t processed_offset = 0;
        remaining_bytes = 0;

        while (bytes_read > 0)
        {
            std::mbstate_t state = std::mbstate_t();
            // Converts a narrow multibyte character to a wide character.
            size_t len = std::mbrtowc(&current_wide_char, file_buf_data_ptr, bytes_read, &state);
            if (len == 0)
            {
                // Ignore this one for now
                bytes_read--;
                file_buf_data_ptr++;
                processed_offset++;
            }
            else if (len >= 1 && len <= bytes_read)
            {
                // Process characters and bytes
                output.m_chars++;
                output.m_bytes += len;
                if (!first_char)
                {
                    previous_wide_char = current_wide_char;
                    first_char = true;
                }
                // Process lines
                if (current_wide_char == L'\n')
                {
                    output.m_lines++;
                }
                // Process words
                if (std::iswspace(current_wide_char) && is_word_under_process)
                {
                    output.m_words++;
                    is_word_under_process = false;
                }
                else if (!std::iswspace(current_wide_char))
                {
                    is_word_under_process = true;
                }

                previous_wide_char = current_wide_char;
                file_buf_data_ptr += len;
                processed_offset += len;
                bytes_read -= len;
            }
            else if (len == static_cast<std::size_t>(-1))
            {
                std::cerr << "Failed: encoding error\n";
                break;
            }
            else if (len == static_cast<std::size_t>(-2))
            {
                // std::cerr << "Failed: incomplete sequence\n";
                remaining_bytes = bytes_read;
                // Move remaining bytes to the start of the buffer
                // for (size_t file_buf_index = processed_offset; file_buf_index < file_buf.size(); file_buf_index++)
                // {
                //     file_buf[file_buf_index - processed_offset] = file_buf[file_buf_index];
                // }
                std::copy(file_buf.begin() + processed_offset, file_buf.end(), file_buf.begin());
                std::fill(file_buf.begin() + file_buf.size() - processed_offset, file_buf.end(), '\0');
                break;
            }
        }
    }
    if (!iswspace(previous_wide_char))
    {
        output.m_words++;
    }
    return true;
}

bool read_file(const Config &config, Output &output)
{
    if (!config.m_input_file_path.empty() 
        && !(std::filesystem::exists(std::filesystem::path{config.m_input_file_path})
            || std::filesystem::is_regular_file(config.m_input_file_path)))
    {
        std::cerr << "Either file doesn't exist or is not a regular file: " << config.m_input_file_path << '\n';
        return false;
    }
    bool ret = false;
    if (config.m_input_file_path.empty())
    {
        ret = process_file(std::cin, output);
    }
    else
    {
        std::ifstream in(config.m_input_file_path, std::ios::binary);
        if (!in.good())
        {
            std::cerr << "Couldn't open file for reading: " << config.m_input_file_path << '\n';
            return false;
        }
        ret = process_file(in, output);
        in.close();
    }
    return ret;
}

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
}   // namespace
int main(int argc, char **argv)
{
    // If the current locale does not support multibyte characters output of -m will match that of -c option
#if defined(_WIN32)
    std::cout << "Current locale is: " << std::setlocale(LC_ALL, ".utf8") << '\n';
#elif defined(__unix__)
    std::cout << "Current locale is: " << std::setlocale(LC_ALL, "") << '\n';
#endif

    Config config;

    int index = 1;
    while (index < argc)
    {
        if (strncmp(argv[index], "-c", 2) == 0)
        {
            config.m_byte_counter = true;
        }
        else if (strncmp(argv[index], "-l", 2) == 0)
        {
            config.m_line_counter = true;
        }
        else if (strncmp(argv[index], "-w", 2) == 0)
        {
            config.m_word_counter = true;
        }
        else if (strncmp(argv[index], "-m", 2) == 0)
        {
            config.m_char_counter = true;
        }
        else if (strncmp(argv[index], "--help", 6) == 0)
        {
            print_usage(argv[0]);
            return 1;
        }
        else
        {
            config.m_input_file_path = std::string{argv[index]};
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

    Output output;
    if (!read_file(config, output))
    {
        return 1;
    }

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