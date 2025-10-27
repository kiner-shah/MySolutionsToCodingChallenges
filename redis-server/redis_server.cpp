#include "Server.hpp"
#include <iostream>

int main()
{
    kredis::Server server;
    try
    {
        server.start();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
}