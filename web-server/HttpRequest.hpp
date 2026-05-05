#pragma once

#include "HttpMethods.hpp"
#include <optional>
#include <string>
#include <string_view>

namespace kweb_server
{
class HttpRequest
{
    HttpMethod m_method;
    std::string m_path;
public:
    HttpRequest(HttpMethod method, const std::string& path);
    static std::optional<HttpRequest> parse(const std::string_view& raw_request);

    bool operator==(const HttpRequest& other) const;
    HttpMethod get_method() const;
    const std::string& get_path() const;
};
} // namespace kweb_server

namespace std
{
template<>
struct hash<kweb_server::HttpRequest>
{
    std::size_t operator()(const kweb_server::HttpRequest& request) const
    {
        std::size_t method_hash = std::hash<int>()(static_cast<int>(request.get_method()));
        std::size_t path_hash = std::hash<std::string>()(request.get_path());
        std::size_t hash = 17;
        hash = hash * 31 + method_hash;
        hash = hash * 31 + path_hash;
        return hash;
    }
};
}   // namespace std