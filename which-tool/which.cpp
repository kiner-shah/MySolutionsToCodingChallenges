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
        auto delimiter_pos = path_sv.find(delimiter);
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

    fs::path program_name_sv{program_name};
    for (const auto& path : candidate_paths)
    {
        if (!fs::exists(path) || !fs::is_directory(path))
        {
            // std::cerr << "Candidate path " << path << " does not exist or is not a directory.\n";
            continue;
        }

        auto it = std::find_if(fs::directory_iterator{path}, fs::directory_iterator{},
            [&program_name_sv](const fs::directory_entry& dir_entry)
            {
                return (dir_entry.is_regular_file() || dir_entry.is_symlink())
                    && (dir_entry.status().permissions() & fs::perms::owner_exec) > fs::perms::none
                    && dir_entry.path().filename() == program_name_sv;
            });

        if (it != fs::directory_iterator{})
        {
            std::cout << it->path().generic_string() << '\n';
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

    auto candidate_paths = split_path_string(path_env_cstr);
    for (int i = 1; i < argc; i++)
    {
        print_program_path(argv[i], candidate_paths);
    }
}