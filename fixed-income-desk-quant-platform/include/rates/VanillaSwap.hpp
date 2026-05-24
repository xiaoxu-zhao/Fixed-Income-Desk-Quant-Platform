#pragma once

#include "rates/YieldCurve.hpp"

#include <string>
#include <vector>

namespace fi::rates {

class VanillaSwap {
public:
    VanillaSwap() = default;
    VanillaSwap(std::string instrumentId,
                double notional,
                double fixedRate,
                double maturityYears,
                int fixedFrequency,
                int floatingFrequency,
                bool payerFixed,
                double position);

    const std::string& instrumentId() const { return instrumentId_; }
    double notional() const { return notional_; }
    double fixedRate() const { return fixedRate_; }
    double maturityYears() const { return maturityYears_; }
    int fixedFrequency() const { return fixedFrequency_; }
    int floatingFrequency() const { return floatingFrequency_; }
    bool payerFixed() const { return payerFixed_; }
    double position() const { return position_; }

    double fixedLegPV(const YieldCurve& discountCurve) const;
    double floatingLegPV(const YieldCurve& discountCurve, const YieldCurve& forwardCurve) const;
    double npv(const YieldCurve& discountCurve, const YieldCurve& forwardCurve) const;
    double parRate(const YieldCurve& discountCurve, const YieldCurve& forwardCurve) const;
    double dv01(const YieldCurve& discountCurve, const YieldCurve& forwardCurve) const;
    double keyRateDv01(const YieldCurve& discountCurve,
                       const YieldCurve& forwardCurve,
                       double keyMaturity) const;

private:
    void validate() const;
    std::vector<double> paymentTimes(int frequency) const;

    std::string instrumentId_;
    double notional_{0.0};
    double fixedRate_{0.0};
    double maturityYears_{0.0};
    int fixedFrequency_{1};
    int floatingFrequency_{1};
    bool payerFixed_{true};
    double position_{0.0};
};

} // namespace fi::rates

