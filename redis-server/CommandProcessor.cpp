#include "CommandProcessor.hpp"

namespace kredis
{
CommandProcessor::CommandProcessor(std::shared_ptr<spdlog::logger> logger)
    : m_logger{std::move(logger)}
{
}

bool CommandProcessor::process(RespType command, RespType& response)
{
    // Clients send commands to a Redis server as an array of bulk strings.
    // The first (and sometimes also the second) bulk string in the array is the command's name.
    // Subsequent elements of the array are the arguments for the command.

    if (!std::holds_alternative<RespArray>(command))
    {
        m_logger->error("Command is not an array");
        return false;
    }
    const auto& array = std::get<RespArray>(command);
    if (array.empty())
    {
        m_logger->error("Command array is empty");
        return false;
    }
    for (const auto& element : array)
    {
        if (std::holds_alternative<RespString>(element))
        {
            const auto& value = std::get<RespString>(element);
            if (!value.m_is_bulk)
            {
                m_logger->error("Command array has an element that isn't a bulk string");
                return false;
            }
        }
        else
        {
            m_logger->error("Command array has an element that isn't a bulk string");
            return false;
        }
    }

    static constexpr const char* invalid_number_arguments = "INVALID_NUMBER_OF_ARGUMENTS";
    //static constexpr const char* key_not_found = "KEY_NOT_FOUND";

    const auto& command_name = std::get<RespString>(array[0]);
    if (command_name.m_str == "PING")
    {
        if (array.size() == 1)
        {
            response = RespString{"PONG", false};
        }
        else
        {
            response = RespError{invalid_number_arguments};
        }
    }
    else if (command_name.m_str == "ECHO")
    {
        if (array.size() == 2)
        {
            const auto& message = std::get<RespString>(array[1]);
            response = RespString{message.m_str, false};
        }
        else
        {
            response = RespError{invalid_number_arguments};
        }
    }
    else
    {
        m_logger->warn("Other commands aren't supported yet");
        return false;
    }

    return true;
}
}   // namespace kredis