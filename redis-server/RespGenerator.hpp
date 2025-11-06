#pragma once

#include <memory>
#include <spdlog/spdlog.h>
#include "RespTypes.hpp"

namespace kredis
{
class RespGenerator
{
    std::shared_ptr<spdlog::logger> m_logger;

    std::size_t calculate_resp_string_size(const RespString& input) const;
    std::size_t calculate_resp_array_size(const RespArray& input) const;
    std::size_t calculate_resp_error_size(const RespError& input) const;
    std::size_t calculate_resp_int_size(const RespInt& input) const;
    std::size_t calculate_resp_null_size(const RespNull& input) const;

    std::string generate_resp_string(const RespString& input) const;
    std::string generate_resp_array(const RespArray& input) const;
    std::string generate_resp_error(const RespError& input) const;
    std::string generate_resp_int(const RespInt& input) const;
    std::string generate_resp_null(const RespNull& input) const;

public:
    RespGenerator(std::shared_ptr<spdlog::logger> logger);

    std::string generate(const RespType& input) const;
    std::string generate(std::shared_ptr<RespType> input) const;
};
}   // namespace kredis