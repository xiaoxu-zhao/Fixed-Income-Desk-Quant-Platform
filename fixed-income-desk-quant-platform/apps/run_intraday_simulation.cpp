#include "core/CsvReader.hpp"
#include "core/Utils.hpp"
#include "execution/LimitOrderBook.hpp"
#include "execution/MarketDataSimulator.hpp"
#include "rates/Portfolio.hpp"
#include "rates/YieldCurve.hpp"

#include <cmath>
#include <iostream>

using fi::core::formatDouble;
using fi::core::joinPath;

int main(int argc, char** argv) {
    try {
        const std::string root = argc > 1 ? argv[1] : ".";
        const std::string outputDir = joinPath(root, "output");
        fi::core::ensureDirectory(outputDir);
        fi::core::ensureDirectory(joinPath(root, "data/market_events"));

        const double tickSize = 1.0 / 64.0;
        const std::string eventsPath = joinPath(root, "data/market_events/simulated_ticks.csv");
        fi::execution::MarketDataSimulator simulator(42);
        simulator.generateToCsv(eventsPath, "ZN", 800, 110.0, tickSize);

        const auto events = fi::execution::readMarketEventsCsv(eventsPath);
        const auto curve = fi::rates::YieldCurve::fromCsv(joinPath(root, "data/curves/curve_yesterday.csv"));
        const auto portfolio = fi::rates::Portfolio::fromCsv(joinPath(root, "data/portfolio/portfolio.csv"));
        const auto keyRisk = portfolio.keyRateDV01(curve, {10.0});
        const double tenYearDv01 = keyRisk.at(10.0);

        fi::execution::LimitOrderBook book;
        std::vector<std::vector<std::string>> rows;
        double initialMid = 0.0;
        bool haveInitialMid = false;
        int cumulativeTradeSize = 0;

        for (const auto& event : events) {
            book.onEvent(event);
            int tradeSize = 0;
            if (event.eventType == fi::execution::EventType::Trade) {
                tradeSize = event.size;
                cumulativeTradeSize += tradeSize;
            }
            const auto snapshot = book.snapshot(event.timestampNs);
            if (!snapshot.bestBid || !snapshot.bestAsk || !std::isfinite(snapshot.midPrice) ||
                !std::isfinite(snapshot.spread) || snapshot.spread <= 0.0) {
                continue;
            }
            if (!haveInitialMid) {
                initialMid = snapshot.midPrice;
                haveInitialMid = true;
            }
            const double yieldMoveBp = -(snapshot.midPrice - initialMid) * 40.0;
            const double intradayPnl = -tenYearDv01 * yieldMoveBp;
            rows.push_back({
                std::to_string(event.timestampNs),
                formatDouble(snapshot.midPrice, 6),
                formatDouble(yieldMoveBp, 4),
                formatDouble(intradayPnl, 2),
                formatDouble(*snapshot.bestBid, 6),
                formatDouble(*snapshot.bestAsk, 6),
                formatDouble(snapshot.spread, 6),
                std::to_string(snapshot.bidDepth),
                std::to_string(snapshot.askDepth),
                std::to_string(tradeSize),
                std::to_string(cumulativeTradeSize)
            });
        }

        fi::core::CsvReader::writeRows(
            joinPath(outputDir, "intraday_pnl.csv"),
            {"timestamp_ns", "mid_price", "implied_yield_move_bp", "intraday_pnl",
             "bid_price", "ask_price", "spread", "bid_depth", "ask_depth",
             "trade_size", "cumulative_trade_size"},
            rows);

        std::cout << "Intraday simulation written to " << eventsPath << " and " << outputDir << "\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "run_intraday_simulation failed: " << ex.what() << "\n";
        return 1;
    }
}
