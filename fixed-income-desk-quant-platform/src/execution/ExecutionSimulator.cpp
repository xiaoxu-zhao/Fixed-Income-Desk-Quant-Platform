#include "execution/ExecutionSimulator.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace fi::execution {

std::vector<Fill> ExecutionSimulator::simulate(const std::vector<MarketSnapshot>& snapshots,
                                               int quantity,
                                               bool buy,
                                               ExecutionStrategy strategy,
                                               int childOrders) const {
    if (quantity <= 0) {
        throw std::invalid_argument("Execution quantity must be positive");
    }
    if (snapshots.empty()) {
        throw std::invalid_argument("Execution simulation requires market snapshots");
    }

    std::vector<Fill> fills;
    if (strategy == ExecutionStrategy::Market) {
        const auto& first = snapshots.front();
        fills.push_back({first.timestampNs, buy ? Side::Ask : Side::Bid, buy ? first.ask : first.bid, quantity});
        return fills;
    }

    const int desiredChildren = std::max(1, childOrders);
    const int interval = std::max(1, static_cast<int>(snapshots.size()) / desiredChildren);
    int remaining = quantity;
    int submittedChildren = 0;

    for (std::size_t i = 0; i < snapshots.size() && remaining > 0 && submittedChildren < desiredChildren;
         i += static_cast<std::size_t>(interval)) {
        const auto& snapshot = snapshots[i];
        const int targetChild = static_cast<int>(std::ceil(static_cast<double>(quantity) / desiredChildren));
        int available = buy ? snapshot.askDepth : snapshot.bidDepth;
        double price = buy ? snapshot.ask : snapshot.bid;

        if (strategy == ExecutionStrategy::Passive) {
            available = snapshot.tradeSize > 0 ? std::min(snapshot.tradeSize, available) : 0;
            price = buy ? snapshot.bid : snapshot.ask;
        }

        const int fillSize = std::min({remaining, targetChild, std::max(0, available)});
        if (fillSize > 0) {
            fills.push_back({snapshot.timestampNs, buy ? Side::Ask : Side::Bid, price, fillSize});
            remaining -= fillSize;
        }
        ++submittedChildren;
    }

    return fills;
}

} // namespace fi::execution

