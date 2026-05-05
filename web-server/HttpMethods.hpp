#pragma once

namespace kweb_server
{
enum class HttpMethod
{
    Get,
    Post,
    Put,
    Patch,
    Delete,
    Options,
    Head,
    Trace,
    Connect
};
}   // namespace kweb_server