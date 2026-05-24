#pragma once

#include <string>
#include <vector>

namespace fi::rates {

class YieldCurve {
public:
    YieldCurve() = default;
    YieldCurve(std::vector<double> maturities, std::vector<double> zeroRates);

    static YieldCurve fromCsv(const std::string& path);

    const std::vector<double>& maturities() const { return maturities_; }
    const std::vector<double>& zeroRates() const { return zeroRates_; }

    double zeroRate(double maturity) const;
    double discountFactor(double maturity) const;
    double forwardRate(double t1, double t2) const;
    YieldCurve bumpedParallel(double bp) const;
    YieldCurve bumpedKeyRate(double keyMaturity, double bp) const;
    void validate() const;

private:
    std::vector<double> maturities_;
    std::vector<double> zeroRates_;
};

} // namespace fi::rates

