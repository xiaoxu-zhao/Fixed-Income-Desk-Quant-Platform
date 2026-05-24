#include "core/CsvReader.hpp"

#include "core/Utils.hpp"

#include <fstream>
#include <stdexcept>
#include <utility>

namespace fi::core {

std::vector<std::string> CsvReader::splitLine(const std::string& line) {
    std::vector<std::string> fields;
    std::string current;
    bool inQuotes = false;

    for (char ch : line) {
        if (ch == '"') {
            inQuotes = !inQuotes;
        } else if (ch == ',' && !inQuotes) {
            fields.push_back(trim(current));
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    fields.push_back(trim(current));
    return fields;
}

std::vector<CsvRow> CsvReader::readRows(const std::string& path) {
    std::ifstream input(path);
    if (!input.is_open()) {
        throw std::runtime_error("Unable to open CSV file: " + path);
    }

    std::string line;
    if (!std::getline(input, line)) {
        return {};
    }

    const auto headers = splitLine(line);
    std::vector<CsvRow> rows;
    int lineNumber = 1;
    while (std::getline(input, line)) {
        ++lineNumber;
        if (trim(line).empty()) {
            continue;
        }
        const auto values = splitLine(line);
        if (values.size() != headers.size()) {
            throw std::runtime_error("CSV column count mismatch in " + path +
                                     " at line " + std::to_string(lineNumber));
        }
        CsvRow row;
        for (std::size_t i = 0; i < headers.size(); ++i) {
            row.emplace(headers[i], values[i]);
        }
        rows.push_back(std::move(row));
    }
    return rows;
}

void CsvReader::writeRows(const std::string& path,
                          const std::vector<std::string>& header,
                          const std::vector<std::vector<std::string>>& rows) {
    std::ofstream output(path);
    if (!output.is_open()) {
        throw std::runtime_error("Unable to write CSV file: " + path);
    }

    for (std::size_t i = 0; i < header.size(); ++i) {
        if (i > 0) {
            output << ",";
        }
        output << header[i];
    }
    output << "\n";

    for (const auto& row : rows) {
        if (row.size() != header.size()) {
            throw std::runtime_error("CSV output row does not match header size for " + path);
        }
        for (std::size_t i = 0; i < row.size(); ++i) {
            if (i > 0) {
                output << ",";
            }
            output << row[i];
        }
        output << "\n";
    }
}

} // namespace fi::core
