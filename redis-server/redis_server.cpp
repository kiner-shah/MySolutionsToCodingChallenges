#include "Server.hpp"
#include <iostream>

namespace
{
void print_usage(const char* program_name)
{
    std::cout << "Usage: " << program_name << " [options]\n";
    std::cout << "\nOptions:\n";
    std::cout << "  --workers <num>    Number of worker threads\n";
    std::cout << "  --port <num>       Port number\n";
    std::cout << "  --help            Show this help message\n";
}
}   // namespace

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        print_usage(argv[0]);
        return 1;
    }
    unsigned int workers = argc >= 2 ? std::stoi(argv[1]) : std::thread::hardware_concurrency();
    unsigned int port = argc >= 3 ? std::stoi(argv[2]) : 6379;
    kredis::Server server{workers, port};
    try
    {
        std::cout << "Starting Redis server\n";
        server.start();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
}