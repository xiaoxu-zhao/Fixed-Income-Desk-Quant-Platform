#include "TestFramework.hpp"

#include "rates/FixedRateBond.hpp"

#include <cmath>

TEST_CASE("Zero coupon bond prices from discounted principal") {
    const fi::rates::YieldCurve curve({1.0, 2.0}, {0.05, 0.05});
    const fi::rates::FixedRateBond bond("ZC1Y", 100.0, 0.0, 1.0, 1, 1.0);
    REQUIRE_APPROX(bond.price(curve), 100.0 * std::exp(-0.05), 1.0e-8);
}

TEST_CASE("Bond price decreases when rates increase") {
    const fi::rates::YieldCurve curve({1.0, 5.0}, {0.04, 0.04});
    const fi::rates::FixedRateBond bond("T5Y", 1000000.0, 0.045, 5.0, 2, 1.0);
    REQUIRE(bond.pv(curve.bumpedParallel(10.0)) < bond.pv(curve));
}

TEST_CASE("Long fixed-rate bond has positive DV01") {
    const fi::rates::YieldCurve curve({1.0, 5.0}, {0.04, 0.04});
    const fi::rates::FixedRateBond bond("T5Y", 1000000.0, 0.045, 5.0, 2, 1.0);
    REQUIRE(bond.dv01(curve) > 0.0);
}

