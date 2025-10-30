#include "DictionaryManager.hpp"
#include "RespErrors.hpp"

namespace kredis
{
void DictionaryManager::set(const std::string &key, const RespType &value)
{
    std::scoped_lock<std::mutex> lock{m_mutex};
    m_dictionary[key] = value;
}

RespType DictionaryManager::get(const std::string &key)
{
    std::scoped_lock<std::mutex> lock{m_mutex};
    auto it = m_dictionary.find(key);
    if (it != m_dictionary.end())
    {
        return it->second;
    }
    return RespError{key_not_found};
}
} // namespace kredis