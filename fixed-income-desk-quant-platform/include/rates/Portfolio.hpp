#pragma once

#include "rates/FixedRateBond.hpp"
#include "rates/VanillaSwap.hpp"

#include <map>
#include <string>
#include <vector>

namespace fi::rates {

struct InstrumentValuation {
    std::string instrumentId;
    std::string type;
    double pvYesterday{0.0};
    double pvToday{0.0};
    double pnl{0.0};
    double dv01{0.0};
    double convexity{0.0};
};

class Portfolio {
public:
    static Portfolio fromCsv(const std::string& path);

    void addBond(const FixedRateBond& bond);
    void addSwap(const VanillaSwap& swap);

    const std::vector<FixedRateBond>& bonds() const { return bonds_; }
    const std::vector<VanillaSwap>& swaps() const { return swaps_; }

    double totalPV(const YieldCurve& curve) const;
    double totalDV01(const YieldCurve& curve) const;
    double totalConvexity(const YieldCurve& curve) const;
    std::map<double, double> keyRateDV01(const YieldCurve& curve,
                                         const std::vector<double>& keyMaturities) const;
    double scenarioPnL(const YieldCurve& baseCurve, const YieldCurve& shockedCurve) const;
    std::vector<InstrumentValuation> instrumentValuations(const YieldCurve& yesterday,
                                                          const YieldCurve& today) const;

private:
    std::vector<FixedRateBond> bonds_;
    std::vector<VanillaSwap> swaps_;
};

} // namespace fi::rates

