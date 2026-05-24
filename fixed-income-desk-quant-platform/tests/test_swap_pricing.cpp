#include "TestFramework.hpp"

#include "rates/VanillaSwap.hpp"

TEST_CASE("Swap par rate produces NPV close to zero") {
    const fi::rates::YieldCurve curve({1.0, 2.0, 5.0, 10.0}, {0.04, 0.04, 0.04, 0.04});
    const fi::rates::VanillaSwap templateSwap("S5Y", 1000000.0, 0.04, 5.0, 2, 4, true, 1.0);
    const double par = templateSwap.parRate(curve, curve);
    const fi::rates::VanillaSwap parSwap("S5Y", 1000000.0, par, 5.0, 2, 4, true, 1.0);
    REQUIRE_APPROX(parSwap.npv(curve, curve), 0.0, 1.0e-6);
}

TEST_CASE("Payer swap has negative DV01 near par") {
    const fi::rates::YieldCurve curve({1.0, 2.0, 5.0, 10.0}, {0.04, 0.04, 0.04, 0.04});
    const fi::rates::VanillaSwap templateSwap("S5Y", 1000000.0, 0.04, 5.0, 2, 4, true, 1.0);
    const double par = templateSwap.parRate(curve, curve);
    const fi::rates::VanillaSwap payer("S5Y", 1000000.0, par, 5.0, 2, 4, true, 1.0);
    REQUIRE(payer.dv01(curve, curve) < 0.0);
}

