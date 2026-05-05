#include "HttpRequestParser.hpp"
#include "HttpMethods.hpp"

namespace
{
std::optional<kweb_server::HttpMethod> parse_method(std::string_view method_str)
{
    if (method_str == "GET")
    {
        return kweb_server::HttpMethod::Get;
    }
    else if (method_str == "POST")
    {
        return kweb_server::HttpMethod::Post;
    }
    else if (method_str == "PUT")
    {
        return kweb_server::HttpMethod::Put;
    }
    else if (method_str == "DELETE")
    {
        return kweb_server::HttpMethod::Delete;
    }
    else if (method_str == "HEAD")
    {
        return kweb_server::HttpMethod::Head;
    }
    else if (method_str == "OPTIONS")
    {
        return kweb_server::HttpMethod::Options;
    }
    else if (method_str == "PATCH")
    {
        return kweb_server::HttpMethod::Patch;
    }
    else if (method_str == "TRACE")
    {
        return kweb_server::HttpMethod::Trace;
    }
    else if (method_str == "CONNECT")
    {
        return kweb_server::HttpMethod::Connect;
    }
    else
    {
        return std::nullopt;
    }
}
}   // namespace

namespace kweb_server
{
std::optional<HttpRequest> parse(const std::array<unsigned char, 2048> &raw_request)
{
    std::string_view input{reinterpret_cast<const char*>(raw_request.data()), raw_request.size()};
    auto pos = input.find("\r\n");
    if (pos == std::string_view::npos)
    {
        return std::nullopt;
    }
    auto request_line = input.substr(0, pos);
    
    // Extract method
    auto method_end_pos = request_line.find(' ');
    if (method_end_pos == std::string_view::npos)
    {
        return std::nullopt;
    }
    auto method_str = request_line.substr(0, method_end_pos);
    auto method_opt = parse_method(method_str);
    if (!method_opt)
    {
        return std::nullopt;
    }
    HttpMethod method = *method_opt;

    // Extract path
    auto path_start_pos = method_end_pos + 1;
    auto path_end_pos = request_line.find(' ', path_start_pos);
    if (path_end_pos == std::string_view::npos)
    {
        return std::nullopt;
    }
    auto path_str = request_line.substr(path_start_pos, path_end_pos - path_start_pos);
    
    // Extract protocol
    auto protocol_start_pos = path_end_pos + 1;
    auto protocol_str = request_line.substr(protocol_start_pos);
    if (protocol_str != "HTTP/1.1")
    {
        return std::nullopt;
    }

    // Extract Host header (optional, but commonly present)
    std::string host_header_value;
    auto headers_start_pos = pos + 2; // Skip past the request line and CRLF
    auto headers_end_pos = input.find("\r\n\r\n", headers_start_pos);
    if (headers_end_pos == std::string_view::npos)
    {
        return std::nullopt;
    }
    auto headers_str = input.substr(headers_start_pos, headers_end_pos - headers_start_pos);
    bool valid_host_found = false;
    std::size_t current_pos = 0;
    while (current_pos < headers_str.size())
    {
        auto line_end_pos = headers_str.find("\r\n", current_pos);
        std::string_view header_line{};
        if (line_end_pos == std::string_view::npos)
        {
            header_line = headers_str.substr(current_pos);
            line_end_pos = headers_str.size();
        }
        else
        {
            header_line = headers_str.substr(current_pos, line_end_pos - current_pos);
        }

        auto colon_pos = header_line.find(':');
        if (colon_pos == std::string_view::npos)
        {
            break;
        }
        if (colon_pos + 1 >= header_line.size())
        {
            break;
        }
        auto header_name = header_line.substr(0, colon_pos);
        if (header_line[colon_pos + 1] == ' ')
        {
            colon_pos++;
        }
        auto header_value = header_line.substr(colon_pos + 1);
        if (header_name == "Host"
            && !header_value.empty()
            && (header_value.starts_with("localhost") || header_value.starts_with("127.0.0.1")))
        {
            valid_host_found = true;
            break;   
        }
        current_pos = line_end_pos + 2; // Move to the start of the next header line
    }
    if (valid_host_found)
    {
        return HttpRequest{method, std::string{path_str}};
    }

    return std::nullopt;
}
}   // namespace kweb_server