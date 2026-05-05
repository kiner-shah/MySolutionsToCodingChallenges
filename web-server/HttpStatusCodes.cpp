#include "HttpStatusCodes.hpp"

namespace kweb_server
{
std::string to_string(HttpStatusCode status_code)
{
    switch (status_code)
    {
    case HttpStatusCode::Ok:
        return "200 OK";
    case HttpStatusCode::BadRequest:
        return "400 Bad Request";
    case HttpStatusCode::NotFound:
        return "404 Not Found";
    case HttpStatusCode::InternalServerError:
        return "500 Internal Server Error";
    default:
        return std::to_string(static_cast<int>(status_code)) + " Unknown Status Code";
    }
}
}   // namespace kweb_server