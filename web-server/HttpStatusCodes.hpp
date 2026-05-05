#pragma once
#include <string>

namespace kweb_server
{
enum class HttpStatusCode
{
    Ok = 200,
    BadRequest = 400,
    NotFound = 404,
    InternalServerError = 500
};

std::string to_string(HttpStatusCode status_code);
}   // namespace kweb_server