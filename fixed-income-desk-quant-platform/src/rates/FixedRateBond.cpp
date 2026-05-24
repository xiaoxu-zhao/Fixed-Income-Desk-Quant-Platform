#include "rates/FixedRateBond.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace fi::rates {

FixedRateBond::FixedRateBond(std::string instrumentId,
                             double face,
                             double couponRate,
                             double maturityYears,
                             int couponFrequency,
                             double position)
    : instrumentId_(std::move(instrumentId)),
      face_(face),
      couponRate_(couponRate),
      maturityYears_(maturityYears),
      couponFrequency_(couponFrequency),
      position_(position) {
    validate();
}

void FixedRateBond::validate() const {
    if (instrumentId_.empty()) {
        throw std::invalid_argument("FixedRateBond requires an instrument_id");
    }
    if (face_ <= 0.0 || maturityYears_ <= 0.0 || couponFrequency_ <= 0) {
        throw std::invalid_argument("FixedRateBond face, maturity and frequency must be positive");
    }
    if (couponRate_ < 0.0) {
        throw std::invalid_argument("FixedRateBond coupon cannot be negative");
    }
}

std::vector<Cashflow> FixedRateBond::cashflows() const {
    const int periods = std::max(1, static_cast<int>(std::round(maturityYears_ * couponFrequency_)));
    const double coupon = face_ * couponRate_ / static_cast<double>(couponFrequency_);
    std::vector<Cashflow> flows;
    flows.reserve(static_cast<std::size_t>(periods));
    for (int i = 1; i <= periods; ++i) {
        const double time = static_cast<double>(i) / static_cast<double>(couponFrequency_);
        double amount = coupon;
        if (i == periods) {
            amount += face_;
        }
        flows.push_back({time, amount});
    }
    return flows;
}

double FixedRateBond::price(const YieldCurve& curve) const {
    double value = 0.0;
    for (const auto& cashflow : cashflows()) {
        value += cashflow.amount * curve.discountFactor(cashflow.time);
    }
    return value;
}

double FixedRateBond::pv(const YieldCurve& curve) const {
    return position_ * price(curve);
}

double FixedRateBond::dv01(const YieldCurve& curve) const {
    // Positive DV01 means the position gains value when yields fall by 1bp.
    const double down = pv(curve.bumpedParallel(-1.0));
    const double up = pv(curve.bumpedParallel(1.0));
    return (down - up) / 2.0;
}

double FixedRateBond::convexity(const YieldCurve& curve) const {
    const double bp = 1.0e-4;
    const double base = pv(curve);
    const double down = pv(curve.bumpedParallel(-1.0));
    const double up = pv(curve.bumpedParallel(1.0));
    return (down + up - 2.0 * base) / (bp * bp);
}

double FixedRateBond::yieldToMaturity(double cleanPrice) const {
    if (cleanPrice <= 0.0) {
        throw std::invalid_argument("clean_price must be positive");
    }
    double low = -0.05;
    double high = 0.25;
    const auto flows = cashflows();

    auto pvAtYield = [&flows](double y) {
        double value = 0.0;
        for (const auto& cashflow : flows) {
            value += cashflow.amount * std::exp(-y * cashflow.time);
        }
        return value;
    };

    for (int i = 0; i < 100; ++i) {
        const double mid = 0.5 * (low + high);
        if (pvAtYield(mid) > cleanPrice) {
            low = mid;
        } else {
            high = mid;
        }
    }
    return 0.5 * (low + high);
}

double FixedRateBond::accruedInterest() const {
    return 0.0;
}

} // namespace fi::rates
