#include "execution/ExecutionAnalytics.hpp"

#include "core/CsvReader.hpp"
#include "core/Utils.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>

namespace fi::execution {

double ExecutionAnalytics::averageFillPrice(const std::vector<Fill>& fills) {
    double notional = 0.0;
    int quantity = 0;
    for (const auto& fill : fills) {
        notional += fill.price * static_cast<double>(fill.size);
        quantity += fill.size;
    }
    return quantity == 0 ? 0.0 : notional / static_cast<double>(quantity);
}

double ExecutionAnalytics::vwap(const std::vector<MarketSnapshot>& snapshots) {
    double weighted = 0.0;
    double weight = 0.0;
    for (const auto& snapshot : snapshots) {
        const int snapshotWeight = snapshot.tradeSize > 0
            ? snapshot.tradeSize
            : std::max(1, snapshot.bidDepth + snapshot.askDepth);
        weighted += snapshot.mid * static_cast<double>(snapshotWeight);
        weight += static_cast<double>(snapshotWeight);
    }
    return weight == 0.0 ? 0.0 : weighted / weight;
}

double ExecutionAnalytics::twap(const std::vector<MarketSnapshot>& snapshots) {
    if (snapshots.empty()) {
        return 0.0;
    }
    double sum = 0.0;
    for (const auto& snapshot : snapshots) {
        sum += snapshot.mid;
    }
    return sum / static_cast<double>(snapshots.size());
}

ExecutionReport ExecutionAnalytics::analyze(const std::vector<Fill>& fills,
                                            const std::vector<MarketSnapshot>& snapshots,
                                            int requestedQuantity,
                                            bool buy,
                                            double tickSize,
                                            double pointValue) {
    if (requestedQuantity <= 0) {
        throw std::invalid_argument("requestedQuantity must be positive");
    }
    if (snapshots.empty()) {
        throw std::invalid_argument("Execution analysis requires market snapshots");
    }
    if (tickSize <= 0.0 || pointValue <= 0.0) {
        throw std::invalid_argument("tickSize and pointValue must be positive");
    }

    int filled = 0;
    for (const auto& fill : fills) {
        filled += fill.size;
    }

    ExecutionReport report;
    report.arrivalPrice = snapshots.front().mid;
    report.averageFillPrice = averageFillPrice(fills);
    report.vwap = vwap(snapshots);
    report.twap = twap(snapshots);
    report.requestedQuantity = requestedQuantity;
    report.filledQuantity = filled;
    report.childOrders = static_cast<int>(fills.size());
    report.fillRatio = static_cast<double>(filled) / static_cast<double>(requestedQuantity);

    if (filled > 0) {
        const double direction = buy ? 1.0 : -1.0;
        report.slippageTicks = direction * (report.averageFillPrice - report.arrivalPrice) / tickSize;
        report.implementationShortfall =
            direction * (report.averageFillPrice - report.arrivalPrice) *
            static_cast<double>(filled) * pointValue;
        report.executionCost =
            direction * (report.averageFillPrice - report.vwap) *
            static_cast<double>(filled) * pointValue;
    }
    return report;
}

void ExecutionAnalytics::writeCsv(const std::string& path, const ExecutionReport& report) {
    fi::core::CsvReader::writeRows(
        path,
        {"metric", "value"},
        {
            {"arrival_price", fi::core::formatDouble(report.arrivalPrice, 6)},
            {"average_fill_price", fi::core::formatDouble(report.averageFillPrice, 6)},
            {"vwap", fi::core::formatDouble(report.vwap, 6)},
            {"twap", fi::core::formatDouble(report.twap, 6)},
            {"slippage_ticks", fi::core::formatDouble(report.slippageTicks, 4)},
            {"implementation_shortfall", fi::core::formatDouble(report.implementationShortfall, 2)},
            {"execution_cost_vs_vwap", fi::core::formatDouble(report.executionCost, 2)},
            {"fill_ratio", fi::core::formatDouble(report.fillRatio, 4)},
            {"requested_quantity", std::to_string(report.requestedQuantity)},
            {"filled_quantity", std::to_string(report.filledQuantity)},
            {"child_orders", std::to_string(report.childOrders)}
        });
}

} // namespace fi::execution
