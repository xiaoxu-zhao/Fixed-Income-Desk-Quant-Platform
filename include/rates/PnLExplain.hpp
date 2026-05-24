#pragma once

#include "rates/Portfolio.hpp"

#include <map>
#include <string>
#include <vector>

namespace fi::rates {

struct PnLExplainResult {
    double fullRevaluationPnL{0.0};
    double parallelRates{0.0};
    double keyRateExplained{0.0};
    double curveTwistSlope{0.0};
    double convexity{0.0};
    double carryApproximation{0.0};
    double residual{0.0};
    std::map<double, double> keyRateMovesBp;
    std::map<double, double> keyRateContributions;
};

class PnLExplain {
public:
    static PnLExplainResult explain(const Portfolio& portfolio,
                                    const YieldCurve& yesterdayCurve,
                                    const YieldCurve& todayCurve,
                                    const std::vector<double>& keyMaturities);
    static void writeCsv(const std::string& path, const PnLExplainResult& result);
};

} // namespace fi::rates

