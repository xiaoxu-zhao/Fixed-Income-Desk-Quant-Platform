#include "core/Utils.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace fi::core {

std::string trim(const std::string& value) {
    const auto first = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) {
        return std::isspace(ch);
    });
    const auto last = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) {
        return std::isspace(ch);
    }).base();
    if (first >= last) {
        return "";
    }
    return std::string(first, last);
}

std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

double toDouble(const std::string& value, const std::string& fieldName) {
    const auto clean = trim(value);
    if (clean.empty()) {
        throw std::invalid_argument("Missing numeric field: " + fieldName);
    }
    std::size_t parsed = 0;
    const double result = std::stod(clean, &parsed);
    if (parsed != clean.size() || !std::isfinite(result)) {
        throw std::invalid_argument("Invalid numeric field " + fieldName + ": " + value);
    }
    return result;
}

int toInt(const std::string& value, const std::string& fieldName) {
    const auto clean = trim(value);
    if (clean.empty()) {
        throw std::invalid_argument("Missing integer field: " + fieldName);
    }
    std::size_t parsed = 0;
    const int result = std::stoi(clean, &parsed);
    if (parsed != clean.size()) {
        throw std::invalid_argument("Invalid integer field " + fieldName + ": " + value);
    }
    return result;
}

long long toLongLong(const std::string& value, const std::string& fieldName) {
    const auto clean = trim(value);
    if (clean.empty()) {
        throw std::invalid_argument("Missing integer field: " + fieldName);
    }
    std::size_t parsed = 0;
    const long long result = std::stoll(clean, &parsed);
    if (parsed != clean.size()) {
        throw std::invalid_argument("Invalid integer field " + fieldName + ": " + value);
    }
    return result;
}

bool toBool(const std::string& value, const std::string& fieldName) {
    const auto clean = toLower(trim(value));
    if (clean == "true" || clean == "1" || clean == "yes" || clean == "y") {
        return true;
    }
    if (clean == "false" || clean == "0" || clean == "no" || clean == "n") {
        return false;
    }
    throw std::invalid_argument("Invalid boolean field " + fieldName + ": " + value);
}

std::string formatDouble(double value, int precision) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(precision) << value;
    return out.str();
}

void ensureDirectory(const std::string& path) {
    std::filesystem::create_directories(std::filesystem::path(path));
}

bool fileExists(const std::string& path) {
    return std::filesystem::exists(std::filesystem::path(path));
}

std::string joinPath(const std::string& left, const std::string& right) {
    return (std::filesystem::path(left) / std::filesystem::path(right)).string();
}

std::vector<double> linspace(double start, double end, int count) {
    if (count <= 0) {
        return {};
    }
    if (count == 1) {
        return {start};
    }
    std::vector<double> values;
    values.reserve(static_cast<std::size_t>(count));
    const double step = (end - start) / static_cast<double>(count - 1);
    for (int i = 0; i < count; ++i) {
        values.push_back(start + step * static_cast<double>(i));
    }
    return values;
}

} // namespace fi::core

