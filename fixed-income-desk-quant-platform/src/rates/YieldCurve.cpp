#include "rates/YieldCurve.hpp"

#include "core/CsvReader.hpp"
#include "core/Utils.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace fi::rates {

YieldCurve::YieldCurve(std::vector<double> maturities, std::vector<double> zeroRates)
    : maturities_(std::move(maturities)), zeroRates_(std::move(zeroRates)) {
    validate();
}

YieldCurve YieldCurve::fromCsv(const std::string& path) {
    const auto rows = fi::core::CsvReader::readRows(path);
    std::vector<double> maturities;
    std::vector<double> rates;
    maturities.reserve(rows.size());
    rates.reserve(rows.size());
    for (const auto& row : rows) {
        maturities.push_back(fi::core::toDouble(row.at("maturity"), "maturity"));
        rates.push_back(fi::core::toDouble(row.at("zero_rate"), "zero_rate"));
    }
    return YieldCurve(std::move(maturities), std::move(rates));
}

void YieldCurve::validate() const {
    if (maturities_.empty()) {
        throw std::invalid_argument("YieldCurve requires at least one maturity pillar");
    }
    if (maturities_.size() != zeroRates_.size()) {
        throw std::invalid_argument("YieldCurve maturities and rates must have equal length");
    }
    for (std::size_t i = 0; i < maturities_.size(); ++i) {
        if (!std::isfinite(maturities_[i]) || maturities_[i] <= 0.0) {
            throw std::invalid_argument("YieldCurve maturities must be positive finite values");
        }
        if (!std::isfinite(zeroRates_[i])) {
            throw std::invalid_argument("YieldCurve zero rates must be finite values");
        }
        if (i > 0 && maturities_[i] <= maturities_[i - 1]) {
            throw std::invalid_argument("YieldCurve maturities must be strictly increasing");
        }
    }
}

double YieldCurve::zeroRate(double maturity) const {
    if (!std::isfinite(maturity) || maturity < 0.0) {
        throw std::invalid_argument("Requested maturity must be a non-negative finite value");
    }
    if (maturity <= maturities_.front()) {
        return zeroRates_.front();
    }
    if (maturity >= maturities_.back()) {
        return zeroRates_.back();
    }

    const auto upper = std::upper_bound(maturities_.begin(), maturities_.end(), maturity);
    const std::size_t idx = static_cast<std::size_t>(upper - maturities_.begin());
    const double t0 = maturities_[idx - 1];
    const double t1 = maturities_[idx];
    const double r0 = zeroRates_[idx - 1];
    const double r1 = zeroRates_[idx];
    const double weight = (maturity - t0) / (t1 - t0);
    return r0 + weight * (r1 - r0);
}

double YieldCurve::discountFactor(double maturity) const {
    if (maturity < 0.0) {
        throw std::invalid_argument("Discount factor maturity cannot be negative");
    }
    if (maturity == 0.0) {
        return 1.0;
    }
    return std::exp(-zeroRate(maturity) * maturity);
}

double YieldCurve::forwardRate(double t1, double t2) const {
    if (t1 < 0.0 || t2 <= t1) {
        throw std::invalid_argument("Forward rate requires 0 <= t1 < t2");
    }
    const double df1 = discountFactor(t1);
    const double df2 = discountFactor(t2);
    return std::log(df1 / df2) / (t2 - t1);
}

YieldCurve YieldCurve::bumpedParallel(double bp) const {
    auto bumped = zeroRates_;
    const double shift = bp * 1.0e-4;
    for (auto& rate : bumped) {
        rate += shift;
    }
    return YieldCurve(maturities_, bumped);
}

YieldCurve YieldCurve::bumpedKeyRate(double keyMaturity, double bp) const {
    if (!std::isfinite(keyMaturity) || keyMaturity <= 0.0) {
        throw std::invalid_argument("Key maturity must be positive and finite");
    }
    auto bumped = zeroRates_;
    const double shift = bp * 1.0e-4;
    std::size_t nearest = 0;
    double nearestDistance = std::numeric_limits<double>::max();
    for (std::size_t i = 0; i < maturities_.size(); ++i) {
        const double distance = std::fabs(maturities_[i] - keyMaturity);
        if (distance < nearestDistance) {
            nearestDistance = distance;
            nearest = i;
        }
    }
    bumped[nearest] += shift;
    return YieldCurve(maturities_, bumped);
}

} // namespace fi::rates
