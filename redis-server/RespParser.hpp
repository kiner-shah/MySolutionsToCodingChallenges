#pragma once

#include <string_view>
#include <variant>
#include <optional>
#include <memory>
#include <string>
#include <spdlog/spdlog.h>
#include "RespTypes.hpp"

namespace kredis
{
enum RespParserState
{
    Incomplete,
    Complete,
    Invalid
};

std::string resp_parser_state_to_string(RespParserState state);

struct RespParserResult
{
    std::optional<RespType> m_type;
    RespParserState m_state;
};

class RespParser
{
    std::shared_ptr<spdlog::logger> m_logger;

    RespParserState parse_simple_string(std::string_view input, RespString& parse_output, std::size_t& parse_end_pos);
    RespParserState parse_bulk_string(std::string_view input, RespString& parse_output, std::size_t& parse_end_pos);
    RespParserState parse_error(std::string_view input, RespError& parse_output, std::size_t& parse_end_pos);
    RespParserState parse_integer(std::string_view input, RespInt& parse_output, std::size_t& parse_end_pos);
    RespParserState parse_array(std::string_view input, RespArray& parse_output, std::size_t& parse_end_pos);
    RespParserState check_null_variant(std::string_view input, std::string_view null_variant, std::size_t& parse_end_pos);
    RespParserState parse_null(std::string_view input, RespNull& parse_output, std::size_t& parse_end_pos);
public:
    RespParser(std::shared_ptr<spdlog::logger> logger);
    bool parse(std::string_view input, std::vector<RespParserResult>& result);
};
}   // namespace kredis
