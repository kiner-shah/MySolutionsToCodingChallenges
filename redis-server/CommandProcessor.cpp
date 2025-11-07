#include "CommandProcessor.hpp"
#include "RespErrors.hpp"

namespace kredis
{

const CommandProcessor::RespTypePtr CommandProcessor::OK_RESPONSE = std::make_shared<RespType>(RespString{"OK", false});
const CommandProcessor::RespTypePtr CommandProcessor::PONG_RESPONSE = std::make_shared<RespType>(RespString{"PONG", false});
const CommandProcessor::RespTypePtr CommandProcessor::NOT_IMPLEMENTED_RESPONSE = std::make_shared<RespType>(RespError{not_implemented});
const CommandProcessor::RespTypePtr CommandProcessor::INVALID_NUMBER_ARGS_RESPONSE = std::make_shared<RespType>(RespError{invalid_number_arguments});

CommandProcessor::CommandProcessor(std::shared_ptr<spdlog::logger> logger)
    : m_logger{std::move(logger)}
{
}

bool CommandProcessor::process(const RespType& command, RespTypePtr& response)
{
    // Clients send commands to a Redis server as an array of bulk strings.
    // The first (and sometimes also the second) bulk string in the array is the command's name.
    // Subsequent elements of the array are the arguments for the command.

    if (!std::holds_alternative<RespArray>(command))
    {
        //m_logger->error("Command is not an array");
        return false;
    }
    const auto& array = std::get<RespArray>(command);
    if (array.empty())
    {
        //m_logger->error("Command array is empty");
        return false;
    }
    for (const auto& element : array)
    {
        if (std::holds_alternative<RespString>(element))
        {
            const auto& value = std::get<RespString>(element);
            if (!value.m_is_bulk)
            {
                //m_logger->error("Command array has an element that isn't a bulk string");
                return false;
            }
        }
        else
        {
            //m_logger->error("Command array has an element that isn't a bulk string");
            return false;
        }
    }

    // By default set this error
    response = INVALID_NUMBER_ARGS_RESPONSE;

    const auto& command_name = std::get<RespString>(array[0]);
    if (command_name.m_str == "CONFIG")
    {
        if (array.size() > 2)
        {
            const auto& config_operation = std::get<RespString>(array[1]);
            if (config_operation.m_str == "GET")
            {
                const auto& property = std::get<RespString>(array[2]);
                if (property.m_str == "appendonly")
                {
                    response = std::make_shared<RespType>(RespArray{RespString{"appendonly", true}, RespString{"no", true}});
                }
                else if (property.m_str == "save")
                {
                    response = std::make_shared<RespType>(RespArray{RespString{"save", true}, RespString{"", true}});
                }
                else
                {
                    response = NOT_IMPLEMENTED_RESPONSE;
                }
            }
            else if (config_operation.m_str == "SET")
            {
                response = NOT_IMPLEMENTED_RESPONSE;
            }
        }
    }
    else if (command_name.m_str == "PING")
    {
        if (array.size() == 1)
        {
            response = CommandProcessor::PONG_RESPONSE;
        }
    }
    else if (command_name.m_str == "ECHO")
    {
        if (array.size() == 2)
        {
            const auto& message = std::get<RespString>(array[1]);
            response = std::make_shared<RespType>(RespString{message.m_str, false});
        }
    }
    else if (command_name.m_str == "GET")
    {
        if (array.size() == 2)
        {
            const auto& key = std::get<RespString>(array[1]);
            response = m_dictionary_manager.get(key.m_str);
        }
    }
    else if (command_name.m_str == "SET")
    {
        if (array.size() == 3)
        {
            const auto& key = std::get<RespString>(array[1]);
            const auto& value = std::get<RespString>(array[2]);
            auto value_ptr = std::make_shared<RespType>(value);
            m_dictionary_manager.set(key.m_str, std::move(value_ptr));
            response = CommandProcessor::OK_RESPONSE;
        }
    }
    else
    {
        //m_logger->warn("Other commands aren't supported yet");
        return false;
    }

    return true;
}
}   // namespace kredis