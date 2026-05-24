#pragma once

#include "rates/Portfolio.hpp"

#include <map>
#include <vector>

namespace fi::rates {

class RiskEngine {
public:
    static double portfolioPV(const Portfolio& portfolio, const YieldCurve& curve);
    static double portfolioDV01(const Portfolio& portfolio, const YieldCurve& curve);
    static double portfolioConvexity(const Portfolio& portfolio, const YieldCurve& curve);
    static std::map<double, double> keyRateDV01(const Portfolio& portfolio,
                                                const YieldCurve& curve,
                                                const std::vector<double>& keyMaturities);
    static double scenarioPnL(const Portfolio& portfolio,
                              const YieldCurve& baseCurve,
                              const YieldCurve& shockedCurve);
};

} // namespace fi::rates

