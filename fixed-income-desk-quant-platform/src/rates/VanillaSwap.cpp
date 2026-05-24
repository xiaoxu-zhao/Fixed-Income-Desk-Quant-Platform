#include "rates/VanillaSwap.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace fi::rates {

VanillaSwap::VanillaSwap(std::string instrumentId,
                         double notional,
                         double fixedRate,
                         double maturityYears,
                         int fixedFrequency,
                         int floatingFrequency,
                         bool payerFixed,
                         double position)
    : instrumentId_(std::move(instrumentId)),
      notional_(notional),
      fixedRate_(fixedRate),
      maturityYears_(maturityYears),
      fixedFrequency_(fixedFrequency),
      floatingFrequency_(floatingFrequency),
      payerFixed_(payerFixed),
      position_(position) {
    validate();
}

void VanillaSwap::validate() const {
    if (instrumentId_.empty()) {
        throw std::invalid_argument("VanillaSwap requires an instrument_id");
    }
    if (notional_ <= 0.0 || maturityYears_ <= 0.0 || fixedFrequency_ <= 0 || floatingFrequency_ <= 0) {
        throw std::invalid_argument("VanillaSwap notional, maturity and frequencies must be positive");
    }
    if (fixedRate_ < -0.05 || fixedRate_ > 0.25) {
        throw std::invalid_argument("VanillaSwap fixed rate is outside a reasonable demo range");
    }
}

std::vector<double> VanillaSwap::paymentTimes(int frequency) const {
    const int periods = std::max(1, static_cast<int>(std::round(maturityYears_ * frequency)));
    std::vector<double> times;
    times.reserve(static_cast<std::size_t>(periods));
    for (int i = 1; i <= periods; ++i) {
        times.push_back(static_cast<double>(i) / static_cast<double>(frequency));
    }
    return times;
}

double VanillaSwap::fixedLegPV(const YieldCurve& discountCurve) const {
    double annuity = 0.0;
    for (const double time : paymentTimes(fixedFrequency_)) {
        annuity += discountCurve.discountFactor(time) / static_cast<double>(fixedFrequency_);
    }
    return notional_ * fixedRate_ * annuity;
}

double VanillaSwap::floatingLegPV(const YieldCurve& discountCurve, const YieldCurve& forwardCurve) const {
    (void)forwardCurve;
    // Version 1 uses a par-floater approximation with the same curve for discounting and forwards.
    return notional_ * (1.0 - discountCurve.discountFactor(maturityYears_));
}

double VanillaSwap::npv(const YieldCurve& discountCurve, const YieldCurve& forwardCurve) const {
    const double fixed = fixedLegPV(discountCurve);
    const double floating = floatingLegPV(discountCurve, forwardCurve);
    const double oneSwapNpv = payerFixed_ ? (floating - fixed) : (fixed - floating);
    return position_ * oneSwapNpv;
}

double VanillaSwap::parRate(const YieldCurve& discountCurve, const YieldCurve& forwardCurve) const {
    (void)forwardCurve;
    double annuity = 0.0;
    for (const double time : paymentTimes(fixedFrequency_)) {
        annuity += discountCurve.discountFactor(time) / static_cast<double>(fixedFrequency_);
    }
    if (annuity <= 0.0) {
        throw std::runtime_error("Swap annuity must be positive");
    }
    return (1.0 - discountCurve.discountFactor(maturityYears_)) / annuity;
}

double VanillaSwap::dv01(const YieldCurve& discountCurve, const YieldCurve& forwardCurve) const {
    const double down = npv(discountCurve.bumpedParallel(-1.0), forwardCurve.bumpedParallel(-1.0));
    const double up = npv(discountCurve.bumpedParallel(1.0), forwardCurve.bumpedParallel(1.0));
    return (down - up) / 2.0;
}

double VanillaSwap::keyRateDv01(const YieldCurve& discountCurve,
                                const YieldCurve& forwardCurve,
                                double keyMaturity) const {
    const double down = npv(discountCurve.bumpedKeyRate(keyMaturity, -1.0),
                            forwardCurve.bumpedKeyRate(keyMaturity, -1.0));
    const double up = npv(discountCurve.bumpedKeyRate(keyMaturity, 1.0),
                          forwardCurve.bumpedKeyRate(keyMaturity, 1.0));
    return (down - up) / 2.0;
}

} // namespace fi::rates
