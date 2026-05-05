#include "HttpResponse.hpp"
#include <algorithm>

namespace kweb_server
{
std::array<unsigned char, 2048> HttpResponse::to_raw_response() const
{
    std::array<unsigned char, 2048> raw_response{};

    std::string response_str = "HTTP/1.1 " + to_string(status_code) + "\r\n";
    if (!body.empty())
    {
        std::string content_length_header = "Content-Length: " + std::to_string(body.size()) + "\r\n";
        response_str += content_length_header + "\r\n";    
    }
    response_str += body + "\r\n";
    std::transform(response_str.begin(), response_str.end(), raw_response.begin(), [](char c) { return static_cast<unsigned char>(c); });
    return raw_response;
}
}   // namespace kweb_server