#pragma once

#include "execution/ExecutionAnalytics.hpp"

#include <vector>

namespace fi::execution {

enum class ExecutionStrategy {
    Market,
    Twap,
    Passive
};

class ExecutionSimulator {
public:
    std::vector<Fill> simulate(const std::vector<MarketSnapshot>& snapshots,
                               int quantity,
                               bool buy,
                               ExecutionStrategy strategy,
                               int childOrders) const;
};

} // namespace fi::execution

