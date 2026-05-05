#pragma once
#include "HttpRequest.hpp"
#include <array>
#include <optional>

namespace kweb_server
{
std::optional<HttpRequest> parse(const std::array<unsigned char, 2048>& raw_request);
}   // namespace kweb_server
