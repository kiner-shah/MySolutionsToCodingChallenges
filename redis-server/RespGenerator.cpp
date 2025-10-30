#include "RespGenerator.hpp"
#include <sstream>

namespace kredis
{
std::string RespGenerator::generate_resp_string(const RespString &input) const
{
    std::stringstream ss;
    if (input.m_is_bulk)
    {
        ss << '$' << input.m_str.size() << "\r\n" << input.m_str << "\r\n";
    }
    else
    {
        ss << '+' << input.m_str << "\r\n";
    }
    return ss.str();
}

std::string RespGenerator::generate_resp_array(const RespArray &input) const
{
    std::stringstream ss;
    ss << '*' << input.size() << "\r\n";
    for (const auto& element : input)
    {
        if (std::holds_alternative<RespString>(element))
        {
            ss << generate_resp_string(std::get<RespString>(element));
        }
        else if (std::holds_alternative<RespInt>(element))
        {
            ss << generate_resp_int(std::get<RespInt>(element));
        }
        else if (std::holds_alternative<RespNull>(element))
        {
            ss << generate_resp_null(std::get<RespNull>(element));
        }
    }
    return ss.str();
}

std::string RespGenerator::generate_resp_error(const RespError &input) const
{
    std::stringstream ss;
    ss << '-' << input << "\r\n";
    return ss.str();
}

std::string RespGenerator::generate_resp_int(const RespInt &input) const
{
    std::stringstream ss;
    ss << ':' << input << "\r\n";
    return ss.str();
}

std::string RespGenerator::generate_resp_null(const RespNull &input) const
{
    std::stringstream ss;
    ss << "_\r\n";
    return ss.str();
}

RespGenerator::RespGenerator(std::shared_ptr<spdlog::logger> logger)
    : m_logger{std::move(logger)}
{
}

std::string RespGenerator::generate(const RespType &input) const
{
    if (std::holds_alternative<RespString>(input))
    {
        m_logger->debug("Generating a RESP string");
        return generate_resp_string(std::get<RespString>(input));
    }
    else if (std::holds_alternative<RespArray>(input))
    {
        return generate_resp_array(std::get<RespArray>(input));
    }
    else if (std::holds_alternative<RespError>(input))
    {
        return generate_resp_error(std::get<RespError>(input));
    }
    else if (std::holds_alternative<RespInt>(input))
    {
        return generate_resp_int(std::get<RespInt>(input));
    }
    else if (std::holds_alternative<RespNull>(input))
    {
        m_logger->debug("Generating a RESP null");
        return generate_resp_null(std::get<RespNull>(input));
    }
    return std::string{};
}

}   // namespace kredis