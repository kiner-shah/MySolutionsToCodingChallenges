#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include "LoadBalancer.hpp"

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " HEALTH_CHECK_PERIOD\n";
        std::cerr << "\nHEALTH_CHECK_PERIOD - period in seconds for health check. Value cannot be negative.\n";
        return 1;
    }
    std::int_least64_t health_check_period_in_seconds = std::stoll(argv[1]);
    if (health_check_period_in_seconds < 0)
    {
        std::cerr << "Error: health check period cannot be negative\n";
        return 1;
    }

    std::vector<std::pair<std::string, std::string>> server_list{
        {"127.0.0.1", "8080"},
        {"127.0.0.1", "8081"},
        {"127.0.0.1", "8082"},
    };

    kload_balancer::LoadBalancer lb{"0.0.0.0", "2000", health_check_period_in_seconds};
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