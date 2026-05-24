#pragma once

#include "execution/MarketEvent.hpp"

#include <map>
#include <string>
#include <vector>

namespace fi::execution {

struct MarketSnapshot {
    long long timestampNs{0};
    double bid{0.0};
    double ask{0.0};
    double mid{0.0};
    double spread{0.0};
    int bidDepth{0};
    int askDepth{0};
    int tradeSize{0};
};

struct Fill {
    long long timestampNs{0};
    Side side{Side::Ask};
    double price{0.0};
    int size{0};
};

struct ExecutionReport {
    double arrivalPrice{0.0};
    double averageFillPrice{0.0};
    double vwap{0.0};
    double twap{0.0};
    double slippageTicks{0.0};
    double implementationShortfall{0.0};
    double fillRatio{0.0};
    double executionCost{0.0};
    int requestedQuantity{0};
    int filledQuantity{0};
    int childOrders{0};
};

class ExecutionAnalytics {
public:
    static double averageFillPrice(const std::vector<Fill>& fills);
    static double vwap(const std::vector<MarketSnapshot>& snapshots);
    static double twap(const std::vector<MarketSnapshot>& snapshots);
    static ExecutionReport analyze(const std::vector<Fill>& fills,
                                   const std::vector<MarketSnapshot>& snapshots,
                                   int requestedQuantity,
                                   bool buy,
                                   double tickSize,
                                   double pointValue);
    static void writeCsv(const std::string& path, const ExecutionReport& report);
};

} // namespace fi::execution

