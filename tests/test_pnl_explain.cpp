#include "TestFramework.hpp"

#include "rates/PnLExplain.hpp"

TEST_CASE("P&L explain preserves rates selloff sign convention") {
    const fi::rates::YieldCurve yesterday({5.0}, {0.04});
    const fi::rates::YieldCurve today({5.0}, {0.0401});
    fi::rates::Portfolio portfolio;
    portfolio.addBond(fi::rates::FixedRateBond("T5Y", 1000000.0, 0.045, 5.0, 2, 1.0));

    const auto explain = fi::rates::PnLExplain::explain(portfolio, yesterday, today, {5.0});

    REQUIRE(explain.fullRevaluationPnL < 0.0);
    REQUIRE(explain.keyRateMovesBp.at(5.0) > 0.0);
    REQUIRE(explain.keyRateContributions.at(5.0) < 0.0);
    REQUIRE(explain.keyRateExplained < 0.0);
}
