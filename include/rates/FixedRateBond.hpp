#pragma once

#include "rates/Cashflow.hpp"
#include "rates/YieldCurve.hpp"

#include <string>
#include <vector>

namespace fi::rates {

class FixedRateBond {
public:
    FixedRateBond() = default;
    FixedRateBond(std::string instrumentId,
                  double face,
                  double couponRate,
                  double maturityYears,
                  int couponFrequency,
                  double position);

    const std::string& instrumentId() const { return instrumentId_; }
    double face() const { return face_; }
    double couponRate() const { return couponRate_; }
    double maturityYears() const { return maturityYears_; }
    int couponFrequency() const { return couponFrequency_; }
    double position() const { return position_; }

    std::vector<Cashflow> cashflows() const;
    double price(const YieldCurve& curve) const;
    double pv(const YieldCurve& curve) const;
    double dv01(const YieldCurve& curve) const;
    double convexity(const YieldCurve& curve) const;
    double yieldToMaturity(double cleanPrice) const;
    double accruedInterest() const;

private:
    void validate() const;

    std::string instrumentId_;
    double face_{0.0};
    double couponRate_{0.0};
    double maturityYears_{0.0};
    int couponFrequency_{1};
    double position_{0.0};
};

} // namespace fi::rates

