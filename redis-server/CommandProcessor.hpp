#pragma once

#include "RespTypes.hpp"
#include "DictionaryManager.hpp"
#include <spdlog/spdlog.h>
#include <memory>

namespace kredis
{
class CommandProcessor
{
    using RespTypePtr = std::shared_ptr<RespType>;

    std::shared_ptr<spdlog::logger> m_logger;
    DictionaryManager m_dictionary_manager;

    static const RespTypePtr OK_RESPONSE;
    static const RespTypePtr PONG_RESPONSE;
    static const RespTypePtr NOT_IMPLEMENTED_RESPONSE;
    static const RespTypePtr INVALID_NUMBER_ARGS_RESPONSE;

public:
    CommandProcessor(std::shared_ptr<spdlog::logger> logger);
    bool process(const RespType& command, RespTypePtr& response);
};
}   // namespace kredis