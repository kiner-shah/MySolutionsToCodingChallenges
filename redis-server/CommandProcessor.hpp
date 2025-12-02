#pragma once

#include "RespTypes.hpp"
#include "DictionaryManager.hpp"
#include <spdlog/spdlog.h>
#include <memory>

namespace kredis
{
class CommandProcessor
{
    std::shared_ptr<spdlog::logger> m_logger;
    std::shared_ptr<DictionaryManager> m_dictionary_manager;

    static const RespTypePtr OK_RESPONSE;
    static const RespTypePtr PONG_RESPONSE;
    static const RespTypePtr NOT_IMPLEMENTED_RESPONSE;
    static const RespTypePtr INVALID_ARG_FORMAT_RESPONSE;
    static const RespTypePtr INVALID_NUMBER_ARGS_RESPONSE;
    static const RespTypePtr INVALID_EXPIRY_VALUE_RESPONSE;
    static const std::uint64_t EXPIRY_CHECKING_PERIOD_SECONDS;

public:
    CommandProcessor(std::shared_ptr<spdlog::logger> logger);
    bool process(const RespType& command, RespTypePtr& response);
};
}   // namespace kredis