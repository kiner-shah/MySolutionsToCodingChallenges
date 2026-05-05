#pragma once
#include "HttpRequest.hpp"
#include <array>
#include <optional>

namespace kweb_server
{
std::optional<HttpRequest> parse(const std::string_view& input);
}   // namespace kweb_server
