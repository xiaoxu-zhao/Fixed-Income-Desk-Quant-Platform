#include "rates/RiskEngine.hpp"

namespace fi::rates {

double RiskEngine::portfolioPV(const Portfolio& portfolio, const YieldCurve& curve) {
    return portfolio.totalPV(curve);
}

double RiskEngine::portfolioDV01(const Portfolio& portfolio, const YieldCurve& curve) {
    return portfolio.totalDV01(curve);
}

double RiskEngine::portfolioConvexity(const Portfolio& portfolio, const YieldCurve& curve) {
    return portfolio.totalConvexity(curve);
}

std::map<double, double> RiskEngine::keyRateDV01(const Portfolio& portfolio,
                                                 const YieldCurve& curve,
                                                 const std::vector<double>& keyMaturities) {
    return portfolio.keyRateDV01(curve, keyMaturities);
}

double RiskEngine::scenarioPnL(const Portfolio& portfolio,
                               const YieldCurve& baseCurve,
                               const YieldCurve& shockedCurve) {
    return portfolio.scenarioPnL(baseCurve, shockedCurve);
}

} // namespace fi::rates

