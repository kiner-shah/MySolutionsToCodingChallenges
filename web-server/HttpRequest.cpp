#include "HttpRequest.hpp"
#include "HttpRequestParser.hpp"

namespace kweb_server
{
HttpRequest::HttpRequest(HttpMethod method, const std::string &path)
    : m_method{method}, m_path{path}
{
}

std::optional<HttpRequest> HttpRequest::parse(const std::array<unsigned char, 2048>& raw_request)
{
    return kweb_server::parse(raw_request);
}

bool HttpRequest::operator==(const HttpRequest &other) const
{
    return m_method == other.m_method && m_path == other.m_path;
}

HttpMethod HttpRequest::get_method() const
{
    return m_method;
}

const std::string& HttpRequest::get_path() const
{
    return m_path;
}
} // namespace kweb_server