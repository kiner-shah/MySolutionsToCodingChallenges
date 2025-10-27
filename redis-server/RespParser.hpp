#pragma once

#include <string_view>
#include <variant>
#include <memory>
#include <spdlog/spdlog.h>
#include "RespTypes.hpp"

namespace kredis
{
class RespParser
{
    std::shared_ptr<spdlog::logger> m_logger;

    bool parse_simple_string(std::string_view input, RespString& parse_output, std::size_t& parse_end_pos);
    bool parse_bulk_string(std::string_view input, RespString& parse_output, std::size_t& parse_end_pos);
    bool parse_error(std::string_view input, RespError& parse_output, std::size_t& parse_end_pos);
    bool parse_integer(std::string_view input, RespInt& parse_output, std::size_t& parse_end_pos);
    bool parse_array(std::string_view input, RespArray& parse_output, std::size_t& parse_end_pos);
    bool parse_null(std::string_view input, RespNull& parse_output, std::size_t& parse_end_pos);
public:
    RespParser(std::shared_ptr<spdlog::logger> logger);
    bool parse(std::string_view input, RespType& result);
};
}   // namespace kredis
