#include "TestFramework.hpp"

#include "execution/ExecutionAnalytics.hpp"

TEST_CASE("Execution analytics compute average fill price and fill ratio") {
    const std::vector<fi::execution::Fill> fills = {
        {0, fi::execution::Side::Ask, 100.25, 4},
        {1, fi::execution::Side::Ask, 100.50, 6}
    };
    const std::vector<fi::execution::MarketSnapshot> snapshots = {
        {0, 99.75, 100.25, 100.00, 0.50, 20, 20, 5},
        {1, 100.00, 100.50, 100.25, 0.50, 20, 20, 5}
    };
    const auto report = fi::execution::ExecutionAnalytics::analyze(
        fills, snapshots, 10, true, 0.25, 1000.0);
    REQUIRE_APPROX(report.averageFillPrice, 100.40, 1.0e-12);
    REQUIRE_APPROX(report.fillRatio, 1.0, 1.0e-12);
    REQUIRE_APPROX(report.slippageTicks, 1.6, 1.0e-12);
}

TEST_CASE("Sell execution below arrival reports positive adverse slippage") {
    const std::vector<fi::execution::Fill> fills = {
        {0, fi::execution::Side::Bid, 99.75, 10}
    };
    const std::vector<fi::execution::MarketSnapshot> snapshots = {
        {0, 99.75, 100.25, 100.00, 0.50, 20, 20, 5}
    };
    const auto report = fi::execution::ExecutionAnalytics::analyze(
        fills, snapshots, 10, false, 0.25, 1000.0);
    REQUIRE_APPROX(report.slippageTicks, 1.0, 1.0e-12);
    REQUIRE(report.implementationShortfall > 0.0);
}

TEST_CASE("Execution analytics compute VWAP and TWAP") {
    const std::vector<fi::execution::MarketSnapshot> snapshots = {
        {0, 99.75, 100.25, 100.00, 0.50, 20, 20, 10},
        {1, 100.75, 101.25, 101.00, 0.50, 20, 20, 30}
    };
    REQUIRE_APPROX(fi::execution::ExecutionAnalytics::twap(snapshots), 100.50, 1.0e-12);
    REQUIRE_APPROX(fi::execution::ExecutionAnalytics::vwap(snapshots), 100.75, 1.0e-12);
}
