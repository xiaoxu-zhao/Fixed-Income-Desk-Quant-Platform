# Methodology Notes

## Treasury Curve Benchmark

The June 2022 FOMC event study uses the on-the-run U.S. Treasury curve as a liquid USD government benchmark and an approximate risk-free curve for rates-risk visualization.

The Treasury curve is a liquid USD government benchmark and an approximate risk-free curve for rates-risk visualization. In practice, banks often use SOFR/OIS curves for collateralized derivatives discounting, while Treasury curves are widely used for government-bond pricing, relative-value analysis, key-rate risk, and spread measurement.

It is not the same as:

- a SOFR/OIS discount curve used for collateralized derivatives
- an interbank unsecured funding curve
- the official Treasury par yield curve

Treasury yields are not perfectly pure risk-free rates. They may include liquidity premium, term premium, safe-haven demand, inflation expectations, supply/demand effects, and tax/regulatory effects. On-the-run Treasuries are very liquid benchmark securities, but they can trade rich because of liquidity and specialness. Therefore an on-the-run curve can differ from the official Treasury par yield curve.

Raw WRDS/CRSP Treasury rows are not published in this repository. The event-study scripts use a private local CSV when it is available and deterministic fallback data otherwise.

## Curve Methodology Comparison

| Curve type | Built from | Meaning | Used for in this project |
|---|---|---|---|
| Treasury par yield curve | Official government par yield data. | Coupon rate that would price a hypothetical Treasury at par for each maturity. | Public benchmark comparison. |
| On-the-run observed YTM curve | WRDS/CRSP on-the-run Treasury issue yields or prices. | Yield to maturity of the most liquid benchmark issues. | Event-study visualization and benchmark curve movement. |
| Fitted zero curve | Dirty prices and cashflows. | Discount rates by maturity that approximately reprice selected Treasury issues. | Pricing, DV01, forward-rate calculation, and P&L explain. |
| SOFR/OIS curve | Overnight indexed swaps / SOFR-linked instruments. | Secured overnight collateralized discount curve. | Future extension for realistic derivatives discounting. |

## Dirty Price

Cashflow pricing should use full price:

```text
dirty_price = TDNOMPRC + TDACCINT
```

`TDNOMPRC` is the clean or nominal price. `TDACCINT` is accrued interest.

## Yield To Maturity

`TDYLD` is CRSP's promised daily yield field. It is the yield that equates future cashflows to full price. It is useful for observed on-the-run YTM diagnostics, but it is not a zero rate.

```text
dirty_price = sum_i cashflow_i / (1 + y / frequency)^(frequency * T_i)
```

## Fitted Zero Curve

Zero rates are discount rates by maturity. A fitted zero curve is calibrated so that model dirty prices approximately match observed dirty prices:

```text
min_z sum_j weight_j * (P_model_j(z) - P_dirty_j)^2
```

The event-study implementation reports an approximate fitted-zero curve from selected on-the-run issue dirty prices and cashflows. It also writes a separate observed-YTM surface for diagnostics.

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

## Government Par Yield

The government Treasury par yield is the coupon rate that would make a hypothetical Treasury trade at par for a given maturity. It is a public benchmark curve, not the same object as a curve built from specific on-the-run issue YTMs or fitted zero rates.

## Level, Slope, And Curvature

The event study reports simple curve factors:

```text
level = average(z_2Y, z_5Y, z_10Y, z_30Y)
slope_10y_2y = z_10Y - z_2Y
curvature_2s5s10s = 2 * z_5Y - z_2Y - z_10Y
```

These factors are summary diagnostics for curve-shape changes and complement the full maturity-by-maturity curve move.

## SOFR/OIS

SOFR/OIS is the more appropriate discount curve for collateralized derivatives and is a future extension for this project. Treasury curves are used here for Treasury bond risk, key-rate DV01 intuition, event-study visualization, and spread benchmarking.

## Why WRDS/On-The-Run Curves May Differ From Government Curves

Possible reasons:

1. Treasury par yield curve is a par curve; WRDS `TDYLD` is individual issue yield to maturity.
2. Fitted zero rates are not YTMs; they are discount rates by maturity.
3. On-the-run bonds can trade rich because of liquidity and special repo demand, lowering their yields.
4. Government par curves are smoothed/interpolated benchmark curves; individual issue curves are based on specific securities.
5. Timing and quote source may differ. WRDS/CRSP fields include `TDSOURCR` and price flags.
6. Clean price vs dirty price matters. Cashflow pricing should use `dirty_price = TDNOMPRC + TDACCINT`.
7. Bills, notes, and bonds can use different yield conventions.
8. Column mapping errors can happen if maturities are inferred by position instead of explicit column names.

If the on-the-run curve differs from the government curve by unusually large amounts, check date filtering, maturity mapping, percent vs decimal scaling, clean vs dirty price, coupon and maturity fields, TIPS/callable filtering, and whether fallback synthetic data were used.
