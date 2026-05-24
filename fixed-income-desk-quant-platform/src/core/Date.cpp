#include "core/Date.hpp"

#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace fi::core {

Date Date::parseIso(const std::string& value) {
    if (value.size() != 10 || value[4] != '-' || value[7] != '-') {
        throw std::invalid_argument("Date must be in YYYY-MM-DD format: " + value);
    }
    Date date;
    date.year = std::stoi(value.substr(0, 4));
    date.month = std::stoi(value.substr(5, 2));
    date.day = std::stoi(value.substr(8, 2));
    if (date.month < 1 || date.month > 12 || date.day < 1 || date.day > 31) {
        throw std::invalid_argument("Invalid calendar date: " + value);
    }
    return date;
}

std::string Date::toString() const {
    std::ostringstream out;
    out << std::setw(4) << std::setfill('0') << year << "-"
        << std::setw(2) << std::setfill('0') << month << "-"
        << std::setw(2) << std::setfill('0') << day;
    return out.str();
}

bool Date::operator==(const Date& other) const {
    return year == other.year && month == other.month && day == other.day;
}

bool Date::operator<(const Date& other) const {
    if (year != other.year) {
        return year < other.year;
    }
    if (month != other.month) {
        return month < other.month;
    }
    return day < other.day;
}

} // namespace fi::core

