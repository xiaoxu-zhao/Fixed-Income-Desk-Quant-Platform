#include "TestFramework.hpp"

#include "rates/YieldCurve.hpp"

TEST_CASE("YieldCurve linear interpolation and flat extrapolation work") {
    const fi::rates::YieldCurve curve({1.0, 2.0, 5.0}, {0.04, 0.05, 0.08});
    REQUIRE_APPROX(curve.zeroRate(1.5), 0.045, 1.0e-12);
    REQUIRE_APPROX(curve.zeroRate(3.5), 0.065, 1.0e-12);
    REQUIRE_APPROX(curve.zeroRate(0.25), 0.04, 1.0e-12);
    REQUIRE_APPROX(curve.zeroRate(30.0), 0.08, 1.0e-12);
}

TEST_CASE("YieldCurve discount factors decrease for positive rates") {
    const fi::rates::YieldCurve curve({1.0, 2.0, 5.0}, {0.03, 0.035, 0.04});
    REQUIRE(curve.discountFactor(1.0) > curve.discountFactor(2.0));
    REQUIRE(curve.discountFactor(2.0) > curve.discountFactor(5.0));
}

TEST_CASE("YieldCurve validates invalid data") {
    bool threw = false;
    try {
        const fi::rates::YieldCurve bad({1.0, 1.0}, {0.03, 0.04});
        (void)bad;
    } catch (const std::exception&) {
        threw = true;
    }
    REQUIRE(threw);
}

