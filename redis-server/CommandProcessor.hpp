#pragma once

#include "RespTypes.hpp"
#include <spdlog/spdlog.h>
#include <memory>

namespace kredis
{
class CommandProcessor
{
    std::shared_ptr<spdlog::logger> m_logger;
public:
    CommandProcessor(std::shared_ptr<spdlog::logger> logger);
    bool process(RespType command, RespType& response);
};
}   // namespace kredis