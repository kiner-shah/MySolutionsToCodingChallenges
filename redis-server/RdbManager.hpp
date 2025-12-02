#pragma once

#include "RespTypes.hpp"
#include "DictionaryValue.hpp"
#include <vector>
#include <optional>

namespace kredis
{
class RdbManager
{
    using DictionarySnapshotType = std::vector<std::pair<std::string, DictionaryValue>>;
    void write_length(std::ofstream& ofile, std::size_t length);
    std::size_t read_length(std::ifstream& ifile);

public:
    bool save(const DictionarySnapshotType& dictionary_snapshot);
    std::optional<DictionarySnapshotType> load();
};
}   // namespace kredis