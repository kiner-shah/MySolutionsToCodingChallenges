#include "utils.hpp"
#include <sstream>

namespace kload_balancer
{
std::array<unsigned char, 2048> construct_health_check_message(const ServerDetails &server)
{
    std::stringstream ss;
    ss << "GET / HTTP/1.1\r\nHost: ";
    if (server.get_ip() == "127.0.0.1")
    {
        ss << "localhost:" << server.get_port();
    }
    else
    {
        ss << server.get_endpoint();    
    }
    ss <<"\r\nUser-Agent: curl/7.81.0";
    ss << "\r\nAccept: */*\r\n\r\n";

    std::string message_template = ss.str();
    return get_buffer_from_message(message_template);
}
}   // namespace kload_balancer