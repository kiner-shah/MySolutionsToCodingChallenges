#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include "LoadBalancer.hpp"

int main(int argc, char** argv)
{
    std::vector<std::pair<std::string, std::string>> server_list{
        {"127.0.0.1", "8080"},
        {"127.0.0.1", "8081"},
        {"127.0.0.1", "8082"},
    };

    kload_balancer::LoadBalancer lb{"0.0.0.0", "2000"};
    std::for_each(server_list.begin(), server_list.end(), [&lb](const std::pair<std::string, std::string>& server)
    {
        const auto& [ip_address, port] = server;
        lb.add_server(ip_address, port);
    });

    try
    {
        lb.start();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
}