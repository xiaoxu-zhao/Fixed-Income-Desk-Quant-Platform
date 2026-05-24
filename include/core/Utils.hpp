#pragma once

#include <string>
#include <vector>

namespace fi::core {

std::string trim(const std::string& value);
std::string toLower(std::string value);
double toDouble(const std::string& value, const std::string& fieldName);
int toInt(const std::string& value, const std::string& fieldName);
long long toLongLong(const std::string& value, const std::string& fieldName);
bool toBool(const std::string& value, const std::string& fieldName);
std::string formatDouble(double value, int precision = 6);
void ensureDirectory(const std::string& path);
bool fileExists(const std::string& path);
std::string joinPath(const std::string& left, const std::string& right);
std::vector<double> linspace(double start, double end, int count);

} // namespace fi::core

