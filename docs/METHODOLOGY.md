# Methodology Notes

## Treasury Curve Benchmark

The June 2022 FOMC event study uses the on-the-run U.S. Treasury curve as a liquid USD government benchmark and an approximate risk-free curve for rates-risk visualization. It is not a perfect pure risk-free curve because Treasury yields can also reflect liquidity, term premia, supply-demand effects, and inflation expectations.

Raw WRDS/CRSP Treasury rows are not published in this repository. The event-study scripts use a private local CSV when it is available and deterministic fallback data otherwise.

## Forward Rates

For zero rates `z(t1)` and `z(t2)` at maturities `t1` and `t2`, the implied forward rate is:

```text
f(t1,t2) = (z(t2) * t2 - z(t1) * t1) / (t2 - t1)
```

## Curve Move In Basis Points

The pre/post FOMC curve move at maturity `T` is:

```text
move_bp(T) = (z_post(T) - z_pre(T)) * 10000
```

A flat curve-move chart indicates a parallel shift. Uneven maturity-by-maturity moves indicate a non-parallel shift.

## Level, Slope, And Curvature

The event study reports simple curve factors:

```text
level = average(z_2Y, z_5Y, z_10Y, z_30Y)
slope_10y_2y = z_10Y - z_2Y
curvature_2s5s10s = 2 * z_5Y - z_2Y - z_10Y
```

These factors are summary diagnostics for curve-shape changes and complement the full maturity-by-maturity curve move.
