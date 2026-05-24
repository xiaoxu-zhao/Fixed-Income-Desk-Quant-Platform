#include "TestFramework.hpp"

#include "rates/Portfolio.hpp"
#include "rates/RiskEngine.hpp"

TEST_CASE("Portfolio PV is sum of instrument PVs") {
    const fi::rates::YieldCurve curve({1.0, 2.0, 5.0, 10.0}, {0.04, 0.04, 0.04, 0.04});
    fi::rates::Portfolio portfolio;
    const fi::rates::FixedRateBond bond("T2Y", 1000000.0, 0.04, 2.0, 2, 3.0);
    const fi::rates::VanillaSwap swap("S5Y_PAY", 1000000.0, 0.041, 5.0, 2, 4, true, 1.0);
    portfolio.addBond(bond);
    portfolio.addSwap(swap);
    const double expected = bond.pv(curve) + swap.npv(curve, curve);
    REQUIRE_APPROX(portfolio.totalPV(curve), expected, 1.0e-6);
}

TEST_CASE("Key-rate DV01 returns one value per key maturity") {
    const fi::rates::YieldCurve curve({1.0, 2.0, 5.0, 10.0, 30.0}, {0.04, 0.04, 0.04, 0.04, 0.04});
    fi::rates::Portfolio portfolio;
    portfolio.addBond(fi::rates::FixedRateBond("T10Y", 1000000.0, 0.045, 10.0, 2, 2.0));
    const auto risk = fi::rates::RiskEngine::keyRateDV01(portfolio, curve, {2.0, 5.0, 10.0, 30.0});
    REQUIRE(risk.size() == 4);
    REQUIRE(risk.at(10.0) > 0.0);
}

TEST_CASE("Single-pillar key-rate DV01 matches total DV01") {
    const fi::rates::YieldCurve curve({5.0}, {0.04});
    fi::rates::Portfolio portfolio;
    portfolio.addBond(fi::rates::FixedRateBond("T5Y", 1000000.0, 0.045, 5.0, 2, 1.0));
    const double total = fi::rates::RiskEngine::portfolioDV01(portfolio, curve);
    const auto keyRisk = fi::rates::RiskEngine::keyRateDV01(portfolio, curve, {5.0});
    REQUIRE_APPROX(keyRisk.at(5.0), total, 1.0e-8);
}
