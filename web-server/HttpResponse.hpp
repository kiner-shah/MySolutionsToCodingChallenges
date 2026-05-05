#pragma once

#include "HttpStatusCodes.hpp"
#include <array>
#include <string>

namespace kweb_server
{
struct HttpResponse
{
    HttpStatusCode status_code;
    std::string body;

    std::string to_raw_response() const;
};
}   // namespace kweb_server