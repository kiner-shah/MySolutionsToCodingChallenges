#include "DictionaryManager.hpp"
#include "RespErrors.hpp"
#include <mutex>

namespace kredis
{
void DictionaryManager::set(const std::string &key, RespTypePtr value)
{
    std::unique_lock<std::shared_mutex> lock{m_shared_mutex};
    m_dictionary[key] = std::move(value);
}

DictionaryManager::RespTypePtr DictionaryManager::get(const std::string &key) const
{
    std::shared_lock<std::shared_mutex> lock{m_shared_mutex};
    auto it = m_dictionary.find(key);
    if (it != m_dictionary.end())
    {
        return it->second;
    }
    return std::make_shared<RespType>(RespError{key_not_found});
}
} // namespace kredis