#pragma once

#include <string>

namespace fi::core {

struct Date {
    int year{1970};
    int month{1};
    int day{1};

    static Date parseIso(const std::string& value);
    std::string toString() const;
    bool operator==(const Date& other) const;
    bool operator<(const Date& other) const;
};

} // namespace fi::core

