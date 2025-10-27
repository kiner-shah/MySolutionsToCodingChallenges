#pragma once

#include <memory>
#include <spdlog/spdlog.h>
#include "RespTypes.hpp"

namespace kredis
{
class RespGenerator
{
    std::shared_ptr<spdlog::logger> m_logger;

    std::string generate_resp_string(const RespString& input) const;
    std::string generate_resp_array(const RespArray& input) const;
    std::string generate_resp_error(const RespError& input) const;
    std::string generate_resp_int(const RespInt& input) const;
    std::string generate_resp_null(const RespNull& input) const;

public:
    RespGenerator(std::shared_ptr<spdlog::logger> logger);

    std::string generate(const RespType& input) const;
};
}   // namespace kredis