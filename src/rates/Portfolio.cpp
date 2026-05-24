#include "rates/Portfolio.hpp"

#include "core/CsvReader.hpp"
#include "core/Utils.hpp"

#include <cctype>
#include <cmath>
#include <stdexcept>

namespace fi::rates {
namespace {

double swapConvexity(const VanillaSwap& swap, const YieldCurve& curve) {
    const double bp = 1.0e-4;
    const double base = swap.npv(curve, curve);
    const double down = swap.npv(curve.bumpedParallel(-1.0), curve.bumpedParallel(-1.0));
    const double up = swap.npv(curve.bumpedParallel(1.0), curve.bumpedParallel(1.0));
    return (down + up - 2.0 * base) / (bp * bp);
}

double maturityFromRowOrId(const fi::core::CsvRow& row, const std::string& instrumentId) {
    const auto explicitMaturity = fi::core::trim(row.at("maturity"));
    if (!explicitMaturity.empty()) {
        return fi::core::toDouble(explicitMaturity, "maturity");
    }
    const auto yPos = instrumentId.find('Y');
    if (yPos == std::string::npos || yPos == 0) {
        throw std::invalid_argument("Swap maturity missing and not inferable from instrument_id: " + instrumentId);
    }
    std::size_t start = yPos;
    while (start > 0) {
        const char ch = instrumentId[start - 1];
        if (!std::isdigit(static_cast<unsigned char>(ch)) && ch != '.') {
            break;
        }
        --start;
    }
    if (start == yPos) {
        throw std::invalid_argument("Swap maturity missing and not inferable from instrument_id: " + instrumentId);
    }
    return std::stod(instrumentId.substr(start, yPos - start));
}

} // namespace

Portfolio Portfolio::fromCsv(const std::string& path) {
    Portfolio portfolio;
    const auto rows = fi::core::CsvReader::readRows(path);
    for (const auto& row : rows) {
        const auto type = fi::core::toLower(row.at("type"));
        const auto id = row.at("instrument_id");
        const double position = fi::core::toDouble(row.at("position"), "position");
        if (type == "bond") {
            portfolio.addBond(FixedRateBond(
                id,
                fi::core::toDouble(row.at("face"), "face"),
                fi::core::toDouble(row.at("coupon"), "coupon"),
                fi::core::toDouble(row.at("maturity"), "maturity"),
                fi::core::toInt(row.at("frequency"), "frequency"),
                position));
        } else if (type == "swap") {
            portfolio.addSwap(VanillaSwap(
                id,
                fi::core::toDouble(row.at("notional"), "notional"),
                fi::core::toDouble(row.at("fixed_rate"), "fixed_rate"),
                maturityFromRowOrId(row, id),
                2,
                4,
                fi::core::toBool(row.at("payer_fixed"), "payer_fixed"),
                position));
        } else {
            throw std::invalid_argument("Unsupported portfolio instrument type: " + type);
        }
    }
    return portfolio;
}

void Portfolio::addBond(const FixedRateBond& bond) {
    bonds_.push_back(bond);
}

void Portfolio::addSwap(const VanillaSwap& swap) {
    swaps_.push_back(swap);
}

double Portfolio::totalPV(const YieldCurve& curve) const {
    double total = 0.0;
    for (const auto& bond : bonds_) {
        total += bond.pv(curve);
    }
    for (const auto& swap : swaps_) {
        total += swap.npv(curve, curve);
    }
    return total;
}

double Portfolio::totalDV01(const YieldCurve& curve) const {
    double total = 0.0;
    for (const auto& bond : bonds_) {
        total += bond.dv01(curve);
    }
    for (const auto& swap : swaps_) {
        total += swap.dv01(curve, curve);
    }
    return total;
}

double Portfolio::totalConvexity(const YieldCurve& curve) const {
    double total = 0.0;
    for (const auto& bond : bonds_) {
        total += bond.convexity(curve);
    }
    for (const auto& swap : swaps_) {
        total += swapConvexity(swap, curve);
    }
    return total;
}

std::map<double, double> Portfolio::keyRateDV01(const YieldCurve& curve,
                                                const std::vector<double>& keyMaturities) const {
    std::map<double, double> result;
    for (const double key : keyMaturities) {
        double total = 0.0;
        const auto curveDown = curve.bumpedKeyRate(key, -1.0);
        const auto curveUp = curve.bumpedKeyRate(key, 1.0);
        for (const auto& bond : bonds_) {
            total += (bond.pv(curveDown) - bond.pv(curveUp)) / 2.0;
        }
        for (const auto& swap : swaps_) {
            total += swap.keyRateDv01(curve, curve, key);
        }
        result[key] = total;
    }
    return result;
}

double Portfolio::scenarioPnL(const YieldCurve& baseCurve, const YieldCurve& shockedCurve) const {
    return totalPV(shockedCurve) - totalPV(baseCurve);
}

std::vector<InstrumentValuation> Portfolio::instrumentValuations(const YieldCurve& yesterday,
                                                                 const YieldCurve& today) const {
    std::vector<InstrumentValuation> values;
    values.reserve(bonds_.size() + swaps_.size());
    for (const auto& bond : bonds_) {
        const double pvYesterday = bond.pv(yesterday);
        const double pvToday = bond.pv(today);
        values.push_back({bond.instrumentId(),
                          "bond",
                          pvYesterday,
                          pvToday,
                          pvToday - pvYesterday,
                          bond.dv01(yesterday),
                          bond.convexity(yesterday)});
    }
    for (const auto& swap : swaps_) {
        const double pvYesterday = swap.npv(yesterday, yesterday);
        const double pvToday = swap.npv(today, today);
        values.push_back({swap.instrumentId(),
                          "swap",
                          pvYesterday,
                          pvToday,
                          pvToday - pvYesterday,
                          swap.dv01(yesterday, yesterday),
                          swapConvexity(swap, yesterday)});
    }
    return values;
}

} // namespace fi::rates
