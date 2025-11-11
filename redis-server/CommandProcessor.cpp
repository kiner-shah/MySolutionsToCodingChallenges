#include "CommandProcessor.hpp"
#include "RespErrors.hpp"
#include "utils.hpp"
#include <charconv>
#include <chrono>

namespace kredis
{
const RespTypePtr CommandProcessor::OK_RESPONSE = std::make_shared<RespType>(RespString{"OK", false});
const RespTypePtr CommandProcessor::NOT_IMPLEMENTED_RESPONSE = std::make_shared<RespType>(RespError{not_implemented});
const RespTypePtr CommandProcessor::PONG_RESPONSE = std::make_shared<RespType>(RespString{"PONG", false});
const RespTypePtr CommandProcessor::INVALID_NUMBER_ARGS_RESPONSE = std::make_shared<RespType>(RespError{invalid_number_arguments});
const RespTypePtr CommandProcessor::INVALID_EXPIRY_VALUE_RESPONSE = std::make_shared<RespType>(RespError{invalid_expiry_value});
const std::uint64_t CommandProcessor::EXPIRY_CHECKING_PERIOD_SECONDS = 10;

CommandProcessor::CommandProcessor(std::shared_ptr<spdlog::logger> logger)
    : m_logger{std::move(logger)},
    m_dictionary_manager{std::make_shared<DictionaryManager>(EXPIRY_CHECKING_PERIOD_SECONDS)}
{
    m_dictionary_manager->start();
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
            response = m_dictionary_manager->get(key.m_str);
        }
    }
    else if (command_name.m_str == "SET")
    {
        if (array.size() >= 3)
        {
            const auto& key = std::get<RespString>(array[1]);
            const auto& value = std::get<RespString>(array[2]);
            auto value_ptr = std::make_shared<RespType>(value);

            std::optional<std::uint64_t> timestamp = std::nullopt;
            if (array.size() > 3)
            {
                const auto& option_name = std::get<RespString>(array[3]);
                const auto& option_value = std::get<RespString>(array[4]);
                std::uint64_t option_value_int = 0;

                if (std::from_chars(
                        option_value.m_str.data(),
                        option_value.m_str.data() + option_value.m_str.size(),
                        option_value_int
                    ).ec != std::errc{})
                {
                    response = INVALID_EXPIRY_VALUE_RESPONSE;
                    return true;
                }

                if (option_name.m_str == "EX")
                {
                    // Expire after N seconds
                    timestamp = get_current_time<std::chrono::milliseconds>() + option_value_int * 1000;
                }
                else if (option_name.m_str == "PX")
                {
                    // Expire after N milliseconds
                    timestamp = get_current_time<std::chrono::milliseconds>() + option_value_int;
                }
                else if (option_name.m_str == "EXAT")
                {
                    // Expire at timestamp in seconds
                    timestamp = get_current_time();
                    if (option_value_int <= timestamp)
                    {
                        response = INVALID_EXPIRY_VALUE_RESPONSE;
                        return true;
                    }
                    timestamp = option_value_int * 1000;
                }
                else if (option_name.m_str == "PXAT")
                {
                    // Expire at timestamp in milliseconds
                    timestamp = get_current_time<std::chrono::milliseconds>();
                    if (option_value_int <= timestamp)
                    {
                        response = INVALID_EXPIRY_VALUE_RESPONSE;
                        return true;
                    }
                    timestamp = option_value_int;
                }
            }

            m_dictionary_manager->set(key.m_str, std::move(value_ptr), timestamp);
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