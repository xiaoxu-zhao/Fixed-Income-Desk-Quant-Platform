#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace fi::core {

using CsvRow = std::unordered_map<std::string, std::string>;

class CsvReader {
public:
    static std::vector<CsvRow> readRows(const std::string& path);
    static std::vector<std::string> splitLine(const std::string& line);
    static void writeRows(const std::string& path,
                          const std::vector<std::string>& header,
                          const std::vector<std::vector<std::string>>& rows);
};

} // namespace fi::core

