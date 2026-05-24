#include "core/CsvReader.hpp"
#include "core/Utils.hpp"
#include "rates/PnLExplain.hpp"
#include "rates/Portfolio.hpp"
#include "rates/RiskEngine.hpp"
#include "rates/YieldCurve.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>

using fi::core::formatDouble;
using fi::core::joinPath;

int main(int argc, char** argv) {
    try {
        const std::string root = argc > 1 ? argv[1] : ".";
        const std::string outputDir = joinPath(root, "output");
        fi::core::ensureDirectory(outputDir);

        const auto yesterday = fi::rates::YieldCurve::fromCsv(joinPath(root, "data/curves/curve_yesterday.csv"));
        const auto today = fi::rates::YieldCurve::fromCsv(joinPath(root, "data/curves/curve_today.csv"));
        const auto portfolio = fi::rates::Portfolio::fromCsv(joinPath(root, "data/portfolio/portfolio.csv"));
        const std::vector<double> keys{2.0, 5.0, 10.0, 30.0};

        const double pvYesterday = portfolio.totalPV(yesterday);
        const double pvToday = portfolio.totalPV(today);
        const double dailyPnl = pvToday - pvYesterday;
        const double totalDv01 = fi::rates::RiskEngine::portfolioDV01(portfolio, yesterday);
        const auto keyDv01 = portfolio.keyRateDV01(yesterday, keys);

        double largestKey = 0.0;
        double largestRisk = 0.0;
        for (const auto& [key, risk] : keyDv01) {
            if (std::fabs(risk) > std::fabs(largestRisk)) {
                largestKey = key;
                largestRisk = risk;
            }
        }

        const std::string hedge = largestRisk > 0.0
            ? "reduce_" + formatDouble(largestKey, 0) + "Y_duration_with_payer_swap_or_futures"
            : "add_" + formatDouble(largestKey, 0) + "Y_duration_with_receiver_swap_or_futures";

        fi::core::CsvReader::writeRows(
            joinPath(outputDir, "daily_report.csv"),
            {"metric", "value"},
            {
                {"pv_yesterday", formatDouble(pvYesterday, 2)},
                {"pv_today", formatDouble(pvToday, 2)},
                {"daily_pnl", formatDouble(dailyPnl, 2)},
                {"total_dv01", formatDouble(totalDv01, 2)},
                {"largest_keyrate_risk", formatDouble(largestKey, 0) + "Y:" + formatDouble(largestRisk, 2)},
                {"suggested_hedge", hedge}
            });

        std::vector<std::vector<std::string>> instrumentRows;
        for (const auto& value : portfolio.instrumentValuations(yesterday, today)) {
            instrumentRows.push_back({
                value.instrumentId,
                value.type,
                formatDouble(value.pvYesterday, 2),
                formatDouble(value.pvToday, 2),
                formatDouble(value.pnl, 2),
                formatDouble(value.dv01, 2),
                formatDouble(value.convexity, 2)
            });
        }
        fi::core::CsvReader::writeRows(
            joinPath(outputDir, "instrument_pv.csv"),
            {"instrument_id", "type", "pv_yesterday", "pv_today", "pnl", "dv01", "convexity"},
            instrumentRows);

        std::vector<std::vector<std::string>> keyRows;
        for (const auto& [key, risk] : keyDv01) {
            keyRows.push_back({formatDouble(key, 0), formatDouble(risk, 2)});
        }
        fi::core::CsvReader::writeRows(
            joinPath(outputDir, "keyrate_dv01.csv"),
            {"key_maturity", "dv01"},
            keyRows);

        const auto explain = fi::rates::PnLExplain::explain(portfolio, yesterday, today, keys);
        fi::rates::PnLExplain::writeCsv(joinPath(outputDir, "pnl_explain.csv"), explain);

        std::cout << "Daily desk report written to " << outputDir << "\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "run_daily_desk_report failed: " << ex.what() << "\n";
        return 1;
    }
}

