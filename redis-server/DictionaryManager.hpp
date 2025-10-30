#pragma once

#include <unordered_map>
#include <mutex>
#include <string>
#include "RespTypes.hpp"

namespace kredis
{
class DictionaryManager
{
    std::unordered_map<std::string, RespType> m_dictionary;
    std::mutex m_mutex;
public:
    void set(const std::string& key, const RespType& value);
    RespType get(const std::string& key);
};
}   // namespace kredis