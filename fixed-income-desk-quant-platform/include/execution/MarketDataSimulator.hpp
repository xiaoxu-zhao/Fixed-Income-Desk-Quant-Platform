#pragma once

#include "execution/MarketEvent.hpp"

#include <string>
#include <vector>

namespace fi::execution {

class MarketDataSimulator {
public:
    explicit MarketDataSimulator(unsigned int seed = 42);
    std::vector<MarketEvent> generate(const std::string& symbol,
                                      int eventCount,
                                      double startPrice,
                                      double tickSize) const;
    void generateToCsv(const std::string& path,
                       const std::string& symbol,
                       int eventCount,
                       double startPrice,
                       double tickSize) const;

private:
    unsigned int seed_{42};
};

} // namespace fi::execution

