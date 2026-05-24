#include "rates/PnLExplain.hpp"

#include "core/CsvReader.hpp"
#include "core/Utils.hpp"

#include <numeric>

namespace fi::rates {

PnLExplainResult PnLExplain::explain(const Portfolio& portfolio,
                                     const YieldCurve& yesterdayCurve,
                                     const YieldCurve& todayCurve,
                                     const std::vector<double>& keyMaturities) {
    PnLExplainResult result;
    const double pvYesterday = portfolio.totalPV(yesterdayCurve);
    const double pvToday = portfolio.totalPV(todayCurve);
    result.fullRevaluationPnL = pvToday - pvYesterday;

    const auto keyDv01 = portfolio.keyRateDV01(yesterdayCurve, keyMaturities);
    double moveSum = 0.0;
    for (const double key : keyMaturities) {
        const double moveBp = (todayCurve.zeroRate(key) - yesterdayCurve.zeroRate(key)) * 10000.0;
        result.keyRateMovesBp[key] = moveBp;
        moveSum += moveBp;
        const double contribution = -keyDv01.at(key) * moveBp;
        result.keyRateContributions[key] = contribution;
        result.keyRateExplained += contribution;
    }

    const double averageMoveBp = keyMaturities.empty() ? 0.0 : moveSum / static_cast<double>(keyMaturities.size());
    result.parallelRates = -portfolio.totalDV01(yesterdayCurve) * averageMoveBp;
    result.curveTwistSlope = result.keyRateExplained - result.parallelRates;

    const double averageMoveDecimal = averageMoveBp * 1.0e-4;
    result.convexity = 0.5 * portfolio.totalConvexity(yesterdayCurve) * averageMoveDecimal * averageMoveDecimal;

    double averageRate = 0.0;
    for (const double key : keyMaturities) {
        averageRate += yesterdayCurve.zeroRate(key);
    }
    if (!keyMaturities.empty()) {
        averageRate /= static_cast<double>(keyMaturities.size());
    }
    result.carryApproximation = pvYesterday * averageRate / 252.0;
    result.residual = result.fullRevaluationPnL - result.keyRateExplained -
                      result.convexity - result.carryApproximation;
    return result;
}

void PnLExplain::writeCsv(const std::string& path, const PnLExplainResult& result) {
    std::vector<std::vector<std::string>> rows = {
        {"full_revaluation_pnl", fi::core::formatDouble(result.fullRevaluationPnL, 2)},
        {"parallel_rates", fi::core::formatDouble(result.parallelRates, 2)},
        {"keyrate_explained", fi::core::formatDouble(result.keyRateExplained, 2)},
        {"curve_twist_slope", fi::core::formatDouble(result.curveTwistSlope, 2)},
        {"convexity", fi::core::formatDouble(result.convexity, 2)},
        {"carry_approximation", fi::core::formatDouble(result.carryApproximation, 2)},
        {"residual", fi::core::formatDouble(result.residual, 2)}
    };
    for (const auto& [key, move] : result.keyRateMovesBp) {
        rows.push_back({"move_bp_" + fi::core::formatDouble(key, 0) + "y",
                        fi::core::formatDouble(move, 4)});
    }
    for (const auto& [key, contribution] : result.keyRateContributions) {
        rows.push_back({"keyrate_contribution_" + fi::core::formatDouble(key, 0) + "y",
                        fi::core::formatDouble(contribution, 2)});
    }
    fi::core::CsvReader::writeRows(path, {"component", "value"}, rows);
}

} // namespace fi::rates

