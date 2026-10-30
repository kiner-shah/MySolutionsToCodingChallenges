#include "Server.hpp"
#include <iostream>

int main()
{
    kredis::Server server;
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