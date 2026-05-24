#include "core/CsvReader.hpp"
#include "core/Utils.hpp"
#include "execution/ExecutionAnalytics.hpp"
#include "execution/ExecutionSimulator.hpp"
#include "execution/LimitOrderBook.hpp"
#include "execution/MarketDataSimulator.hpp"

#include <cmath>
#include <iostream>

using fi::core::formatDouble;
using fi::core::joinPath;

namespace {

std::vector<fi::execution::MarketSnapshot> replaySnapshots(const std::vector<fi::execution::MarketEvent>& events) {
    fi::execution::LimitOrderBook book;
    std::vector<fi::execution::MarketSnapshot> snapshots;
    for (const auto& event : events) {
        book.onEvent(event);
        const auto top = book.snapshot(event.timestampNs);
        if (!top.bestBid || !top.bestAsk || !std::isfinite(top.midPrice) ||
            !std::isfinite(top.spread) || top.spread <= 0.0) {
            continue;
        }
        snapshots.push_back({
            event.timestampNs,
            *top.bestBid,
            *top.bestAsk,
            top.midPrice,
            top.spread,
            top.bidDepth,
            top.askDepth,
            event.eventType == fi::execution::EventType::Trade ? event.size : 0
        });
    }
    return snapshots;
}

} // namespace

int main(int argc, char** argv) {
    try {
        const std::string root = argc > 1 ? argv[1] : ".";
        const std::string outputDir = joinPath(root, "output");
        fi::core::ensureDirectory(outputDir);

        const double tickSize = 1.0 / 64.0;
        const double pointValue = 1000.0;
        const std::string eventsPath = joinPath(root, "data/market_events/simulated_ticks.csv");
        if (!fi::core::fileExists(eventsPath)) {
            fi::execution::MarketDataSimulator(42).generateToCsv(eventsPath, "ZN", 800, 110.0, tickSize);
        }

        const auto events = fi::execution::readMarketEventsCsv(eventsPath);
        const auto snapshots = replaySnapshots(events);
        fi::execution::ExecutionSimulator simulator;
        const int requestedQuantity = 500;
        const bool buy = false;
        const auto fills = simulator.simulate(
            snapshots, requestedQuantity, buy, fi::execution::ExecutionStrategy::Twap, 25);
        const auto report = fi::execution::ExecutionAnalytics::analyze(
            fills, snapshots, requestedQuantity, buy, tickSize, pointValue);
        fi::execution::ExecutionAnalytics::writeCsv(joinPath(outputDir, "execution_report.csv"), report);

        std::cout << "Full desk day execution analytics written to " << outputDir << "\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "run_full_desk_day failed: " << ex.what() << "\n";
        return 1;
    }
}
