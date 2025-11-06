#pragma once

#include <unordered_map>
#include <shared_mutex>
#include <string>
#include <memory>
#include "RespTypes.hpp"

namespace kredis
{
class DictionaryManager
{
    using RespTypePtr = std::shared_ptr<RespType>;

    std::unordered_map<std::string, RespTypePtr> m_dictionary;
    mutable std::shared_mutex m_shared_mutex;
public:
    void set(const std::string& key, RespTypePtr value);
    RespTypePtr get(const std::string& key) const;
};
}   // namespace kredis