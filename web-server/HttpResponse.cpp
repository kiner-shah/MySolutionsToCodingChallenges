#include "HttpResponse.hpp"
#include <algorithm>

namespace kweb_server
{
std::string HttpResponse::to_raw_response() const
{
    std::string response_str = "HTTP/1.1 " + to_string(status_code) + "\r\n";
    if (!body.empty())
    {
        std::string content_length_header = "Content-Length: " + std::to_string(body.size()) + "\r\n";
        response_str += content_length_header + "\r\n";    
    }
    response_str += body + "\r\n";
    return response_str;
}
}   // namespace kweb_server