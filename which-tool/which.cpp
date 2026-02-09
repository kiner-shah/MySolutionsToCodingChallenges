#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string_view>
#include <vector>

namespace
{
std::vector<std::string_view> split_path_string(std::string_view path_sv)
{
#if defined(_WIN32)
    const char delimiter = ';';
#elif defined(__unix__)
    const char delimiter = ':';
#endif
    std::vector<std::string_view> candidate_paths;
    while (true)
    {
        const std::size_t delimiter_pos = path_sv.find(delimiter);
        if (delimiter_pos == std::string_view::npos)
        {
            candidate_paths.push_back(path_sv.substr(0));
            break;
        }
        else
        {
            candidate_paths.push_back(path_sv.substr(0, delimiter_pos));
            path_sv = path_sv.substr(delimiter_pos + 1);
        }
    }
    return candidate_paths;
}

void print_program_path(const char* program_name, const std::vector<std::string_view>& candidate_paths)
{
    namespace fs = std::filesystem;

    const fs::path program_name_sv{program_name};
    for (const auto& path : candidate_paths)
    {
        fs::path program_path{path};
        program_path /= program_name;

        const fs::file_status program_status = fs::status(program_path);
        // Note: We don't need to check if the file exists.
        // fs::exists(program_path) only checks its file_status.type() (obtained by fs::status(program_path))
        // and returns true if it's anything other than fs::file_type::none or fs::file_type::not_found.
        if ((program_status.type() == fs::file_type::regular || program_status.type() == fs::file_type::symlink)
            && (program_status.permissions() & fs::perms::owner_exec) > fs::perms::none)
        {
            std::cout << program_path.generic_string() << '\n';
            break;
        }
    }
}
}   // namespace

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " program ...\n";
        return EXIT_FAILURE;
    }

    const char* path_env_cstr = std::getenv("PATH");
    if (!path_env_cstr)
    {
        std::cerr << "PATH environment variable is not set.\n";
        return EXIT_FAILURE;
    }

    const auto candidate_paths = split_path_string(path_env_cstr);
    for (int i = 1; i < argc; i++)
    {
        print_program_path(argv[i], candidate_paths);
    }
}