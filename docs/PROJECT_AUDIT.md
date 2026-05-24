# Project Audit

## 1. What Currently Works

The current implementation is a working Phase 1 fixed-income desk quant simulation.

- C++17 engine with a clean `include/`, `src/`, `apps/`, and `tests/` split.
- CMake build with a default offline CTest-compatible test harness.
- Synthetic Treasury zero curves loaded from CSV.
- Treasury bond pricing with PV, DV01, convexity, and yield-to-maturity support in the bond component.
- Vanilla swap pricing with NPV, par-rate logic, DV01, and key-rate DV01.
- Portfolio aggregation for PV, instrument-level valuation, total DV01, convexity, scenario P&L, and nearest-pillar key-rate DV01.
- Daily P&L explain with full revaluation, key-rate attribution, parallel-rate approximation, curve twist/slope, convexity, carry approximation, and residual.
- Deterministic synthetic Treasury futures market replay using a fixed seed.
- Aggregate limit order book with order-id aware add, cancel, and trade handling.
- Intraday P&L linkage from synthetic ZN futures mid-price changes to approximate 10Y yield shocks through 10Y key-rate DV01.
- Basic execution analytics including arrival price, average fill, VWAP, TWAP, slippage in ticks, implementation shortfall, execution cost versus VWAP, fill ratio, requested quantity, filled quantity, and child order count.
- Python visualization/dashboard workflow that reads generated CSVs and produces PNG plots plus `output/dashboard.html`.
- Rule-based desk summary in `output/desk_summary.md`.
- Deterministic demo script that rebuilds, tests, regenerates reports, plots, dashboard, and summary.

## 2. What Is Missing for a Stronger Desk Quant Showcase

The next high-value improvements should deepen the analytics without rewriting the current project.

- Corporate bond spread analytics: YTM, G-spread, Z-spread, CS01, spread curves, and credit P&L explain.
- Rates versus spread decomposition for corporate bond positions.
- Rich/cheap signal for identifying bonds trading wide or tight versus a fitted spread curve.
- Scenario and stress testing library covering parallel shocks, steepeners, flatteners, belly shocks, spread widening, and combined rates plus credit shocks.
- Hedge optimizer that recommends Treasury futures or swap hedges and reports pre/post hedge key-rate DV01.
- Market data validation layer for curve sanity checks, price/spread outliers, missing points, stale data, and crossed markets.
- Enhanced TCA with slippage by order size, market order walking the book, passive versus aggressive execution comparison, and a latency report.
- pybind11 bindings so Python dashboards and notebooks can call the C++ pricing/risk/order-book/TCA engine directly.
- Optional real-data pipeline for public Treasury, FRED, TRACE-style sample data, or user-supplied CSVs.
- AI/RAG credit assistant for evidence-backed credit memos, where retrieval and LLMs explain evidence while transparent models handle numerical fair-spread prediction.

## 3. Known Simplifications

The current system is intentionally simplified for clarity and reproducibility.

- Curves are synthetic zero curves and are not bootstrapped from bills, notes, bonds, futures, swaps, OIS, or SOFR instruments.
- Interpolation is linear and extrapolation is flat.
- Calendars, holidays, settlement conventions, accrued interest, ex-coupon periods, business-day adjustments, and detailed day-count conventions are simplified.
- Bond cashflows are simplified and do not model every street convention for Treasury settlement and accrued interest.
- Swaps use a simplified single-curve framework and a par-floating-leg approximation.
- P&L explain is useful for demonstration but not a full production attribution engine.
- Carry is an approximation.
- Futures-to-yield conversion is deterministic and stylized rather than based on CTD, duration, basis, or invoice pricing.
- The order book stores aggregate displayed depth by price level and tracks order ids for add/cancel/trade events, but it is not a complete exchange simulator.
- Execution simulation does not yet model queue priority, hidden liquidity, venue routing, child-order scheduling details, or realistic latency.
- Market data is synthetic and deterministic; there is no live feed or historical market-data adapter.

## 4. Suggested Next Development Milestones

The strongest next milestone is Phase 2: corporate bond credit analytics.

1. Add a corporate bond instrument type to the portfolio schema while preserving the existing bond and swap flow.
2. Implement YTM, G-spread, Z-spread, and CS01 for corporate bonds.
3. Add a simple corporate spread curve and spread interpolation.
4. Extend daily reporting with rates DV01, CS01, Z-spread, and rates versus spread P&L explain.
5. Add focused C++ tests for spread solving, CS01 sign convention, and credit P&L attribution.
6. Add Python plots for spread curve, CS01 by issuer/sector, and rates versus credit P&L explain.
7. Update the desk summary to call out whether P&L came from rates, credit spread moves, curve shape, or residual.

After that, Phase 3 should add stress testing and hedge optimization, because it turns the project from a reporting demo into a decision-support tool.

## 5. Interview Talking Points

- The project uses a C++17 analytics backend and Python reporting layer, which mirrors a common front-office architecture: C++ for deterministic pricing/risk/order-book/TCA, Python for workflow, visualization, and reports.
- The demo connects several desk workflows instead of showing isolated formulas: curve update, pricing, risk, P&L explain, futures market replay, execution analytics, plots, dashboard, and summary.
- Key-rate DV01 is a useful rates-desk feature because it shows where curve exposure sits instead of only reporting one aggregate DV01; in Phase 1 it is a nearest-pillar bump, not a production bucketed risk model.
- The daily P&L explain uses full revaluation as the anchor, then compares first-order key-rate attribution, parallel-rate effect, curve-shape effect, convexity, carry, and residual. It is intentionally explanatory, not a production additive attribution waterfall.
- The synthetic electronic trading workflow demonstrates market microstructure awareness: event stream, order book, top-of-book, spread, depth, market replay, and execution quality metrics.
- The deterministic design is deliberate: fixed synthetic inputs and a fixed market-data seed make builds, tests, plots, and reports reproducible.
- The current limitations are explainable and form a credible roadmap: corporate spread analytics, scenario testing, hedge optimization, validation, enhanced TCA, pybind11, real-data adapters, and AI/RAG credit research.
- A good design principle for future phases is to keep numerical pricing and risk in C++, while letting Python orchestrate research workflows, charts, dashboards, ML experiments, and RAG-based reporting.
