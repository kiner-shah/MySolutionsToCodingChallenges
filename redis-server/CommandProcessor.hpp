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
    DictionaryManager m_dictionary_manager;
public:
    using RespTypePtr = std::shared_ptr<RespType>;

    CommandProcessor(std::shared_ptr<spdlog::logger> logger);
    bool process(const RespType& command, RespTypePtr& response);
};
}   // namespace kredis