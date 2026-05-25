from __future__ import annotations

import argparse
import math
import os
from pathlib import Path

import numpy as np
import pandas as pd


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_PRIVATE_CSV = Path("C:/Users/xzhaox/Downloads/treasuries.csv")
EVENT_DIR = ROOT / "output" / "event_study"

START_DATE = "2022-06-01"
END_DATE = "2022-06-28"
PRE_EVENT_DATE = "2022-06-14"
POST_EVENT_DATE = "2022-06-16"

MATURITIES = np.array([0.25, 0.50, 1.00, 2.00, 3.00, 5.00, 7.00, 10.00, 20.00, 30.00])
FORWARD_BUCKETS = list(zip(MATURITIES[:-1], MATURITIES[1:]))


COLUMN_DIAGNOSTICS = [
    ("clean_price", "TDNOMPRC", "Clean or nominal price", "dirty price and yield diagnostics"),
    ("accrued_interest", "TDACCINT", "Accrued interest", "dirty price"),
    ("dirty_price", "TDNOMPRC + TDACCINT", "Full price used for cashflow discounting", "zero curve fitting"),
    ("bid", "TDBID", "Daily bid quote", "bid/ask diagnostics"),
    ("ask", "TDASK", "Daily ask quote", "bid/ask diagnostics"),
    ("mid_price", "(TDBID + TDASK)/2 if available", "Bid/ask midpoint", "optional pricing input"),
    ("yield_to_maturity", "TDYLD", "CRSP promised daily yield", "observed YTM diagnostics"),
    ("duration", "TDDURATN", "Macaulay duration", "risk diagnostics"),
    ("price_source", "TDSOURCR", "Quote source", "data quality diagnostics"),
    ("price_flag", "TDNOMPRC_FLG", "Price availability flag", "data quality diagnostics"),
    ("tips_index_ratio", "TDIDXRATIO", "TIPS indicator", "exclude TIPS from nominal curve"),
]


def maturity_label(years: float) -> str:
    if years < 1.0:
        return f"{int(round(years * 12))}M"
    return f"{years:g}Y"


def resolve_input_path(cli_path: str | None = None) -> Path | None:
    if cli_path:
        path = Path(cli_path)
        return path if path.exists() else None
    raw_env = os.environ.get("FIQ_TREASURIES_CSV")
    if raw_env:
        env_path = Path(raw_env)
        if env_path.exists():
            return env_path
    return DEFAULT_PRIVATE_CSV if DEFAULT_PRIVATE_CSV.exists() else None


def write_column_diagnostics() -> None:
    EVENT_DIR.mkdir(parents=True, exist_ok=True)
    frame = pd.DataFrame(COLUMN_DIAGNOSTICS, columns=["field_used", "source_column", "description", "used_for"])
    frame.to_csv(EVENT_DIR / "on_the_run_column_diagnostics.csv", index=False)


def read_private_treasuries(path: Path) -> pd.DataFrame:
    needed = {
        "KYTREASNO",
        "TREASNO",
        "KYCRSPID",
        "CRSPID",
        "TCUSIP",
        "CALDT",
        "TMATDT",
        "TCOUPRT",
        "TNIPPY",
        "ITYPE",
        "TDNOMPRC",
        "TDACCINT",
        "TDBID",
        "TDASK",
        "TDYLD",
        "TDDURATN",
        "TDSOURCR",
        "TDNOMPRC_FLG",
        "TDIDXRATIO",
    }
    frame = pd.read_csv(path, usecols=lambda col: col in needed)
    required = {"CALDT", "TMATDT", "TDNOMPRC", "TDACCINT", "TDYLD"}
    missing = required.difference(frame.columns)
    if missing:
        raise ValueError(f"Missing required Treasury columns: {sorted(missing)}")
    return frame


def numeric(frame: pd.DataFrame, column: str, default: float = np.nan) -> pd.Series:
    if column not in frame:
        return pd.Series(default, index=frame.index, dtype="float64")
    return pd.to_numeric(frame[column], errors="coerce")


def text(frame: pd.DataFrame, column: str, default: str = "") -> pd.Series:
    if column not in frame:
        return pd.Series(default, index=frame.index, dtype="object")
    return frame[column].fillna(default).astype(str)


def annualize_tdyld(tdyld: pd.Series) -> tuple[pd.Series, float]:
    median_abs = tdyld.dropna().abs().median()
    annualization = 365.25 if pd.notna(median_abs) and median_abs < 0.01 else 1.0
    return tdyld * annualization, annualization


def zero_equivalent_rate(dirty_price: float, coupon_rate: float, maturity: float, frequency: float) -> float:
    if not np.isfinite(dirty_price) or not np.isfinite(maturity) or dirty_price <= 0.0 or maturity <= 0.0:
        return np.nan

    if not np.isfinite(frequency) or frequency <= 0.0:
        frequency = 2.0 if coupon_rate > 0.0 else 1.0

    coupon_rate = max(coupon_rate, 0.0)
    if coupon_rate == 0.0:
        return -math.log(dirty_price / 100.0) / maturity

    coupon = 100.0 * coupon_rate / frequency
    payment_times = [i / frequency for i in range(1, int(math.floor(maturity * frequency)) + 1)]
    if not payment_times or abs(payment_times[-1] - maturity) > 1e-6:
        payment_times.append(maturity)
    payment_times = sorted(t for t in payment_times if t > 1e-8)
    cashflows = [coupon] * len(payment_times)
    cashflows[-1] += 100.0

    def pv(rate: float) -> float:
        return sum(cf * math.exp(-rate * t) for cf, t in zip(cashflows, payment_times))

    low, high = -0.10, 0.25
    if pv(low) < dirty_price:
        return low
    if pv(high) > dirty_price:
        return high
    for _ in range(80):
        mid = (low + high) / 2.0
        if pv(mid) > dirty_price:
            low = mid
        else:
            high = mid
    return (low + high) / 2.0


def build_input_check_from_private(path: Path) -> tuple[pd.DataFrame, dict[str, object]]:
    raw = read_private_treasuries(path)
    raw["CALDT"] = pd.to_datetime(raw["CALDT"], errors="coerce")
    raw["TMATDT"] = pd.to_datetime(raw["TMATDT"], errors="coerce")
    raw = raw[(raw["CALDT"] >= START_DATE) & (raw["CALDT"] <= END_DATE)].copy()

    clean_price = numeric(raw, "TDNOMPRC")
    accrued = numeric(raw, "TDACCINT").fillna(0.0)
    bid = numeric(raw, "TDBID")
    ask = numeric(raw, "TDASK")
    tdyld_raw = numeric(raw, "TDYLD")
    tdyld, annualization = annualize_tdyld(tdyld_raw)
    duration = numeric(raw, "TDDURATN")
    coupon_rate = numeric(raw, "TCOUPRT").fillna(0.0) / 100.0
    frequency = numeric(raw, "TNIPPY").fillna(2.0)
    maturity_years = (raw["TMATDT"] - raw["CALDT"]).dt.days / 365.25
    dirty_price = clean_price + accrued
    bid_ask_spread = ask - bid
    tips_index_ratio = numeric(raw, "TDIDXRATIO")
    itype = numeric(raw, "ITYPE")
    is_tips = tips_index_ratio.notna() | itype.isin([11, 12, 13])

    check = pd.DataFrame(
        {
            "date": raw["CALDT"].dt.strftime("%Y-%m-%d"),
            "treasno": text(raw, "KYTREASNO", "").where(text(raw, "KYTREASNO", "") != "", text(raw, "TREASNO", "")),
            "crspid": text(raw, "CRSPID", "").where(text(raw, "CRSPID", "") != "", text(raw, "KYCRSPID", "")),
            "maturity_years": maturity_years,
            "coupon_rate": coupon_rate,
            "clean_price": clean_price,
            "accrued_interest": accrued,
            "dirty_price": dirty_price,
            "bid": bid,
            "ask": ask,
            "bid_ask_spread": bid_ask_spread,
            "tdyld": tdyld,
            "duration": duration,
            "price_source": text(raw, "TDSOURCR", ""),
            "price_flag": text(raw, "TDNOMPRC_FLG", ""),
            "tips_index_ratio": tips_index_ratio,
            "frequency": frequency,
            "used_in_curve": False,
            "exclusion_reason": "",
        }
    )

    reasons: list[str] = []
    for idx, row in check.iterrows():
        row_reasons = []
        if not isinstance(row["date"], str) or row["date"] == "NaT":
            row_reasons.append("missing date")
        if not np.isfinite(row["maturity_years"]) or row["maturity_years"] <= 0.02 or row["maturity_years"] > 31.0:
            row_reasons.append("outside maturity window")
        if not np.isfinite(row["clean_price"]) or row["clean_price"] <= 0.0:
            row_reasons.append("invalid clean price")
        if not np.isfinite(row["dirty_price"]) or row["dirty_price"] <= 0.0:
            row_reasons.append("invalid dirty price")
        if not np.isfinite(row["tdyld"]) or row["tdyld"] < -0.02 or row["tdyld"] > 0.20:
            row_reasons.append("invalid yield scale")
        if bool(is_tips.loc[idx]):
            row_reasons.append("TIPS excluded")
        if row_reasons:
            check.at[idx, "exclusion_reason"] = "; ".join(row_reasons)
        reasons.extend(row_reasons)

    zero_rates = [
        zero_equivalent_rate(row.dirty_price, row.coupon_rate, row.maturity_years, row.frequency)
        for row in check.itertuples(index=False)
    ]
    check["fitted_zero_rate"] = zero_rates
    meta = {
        "source": "private",
        "source_detail": str(path),
        "tdyld_annualization": annualization,
        "excluded_tips_count": int(sum("TIPS excluded" in reason for reason in check["exclusion_reason"])),
        "invalid_price_count": int(check["exclusion_reason"].str.contains("invalid .*price", regex=True).sum()),
    }
    return check, meta


def build_fallback_input_check() -> tuple[pd.DataFrame, dict[str, object]]:
    dates = pd.bdate_range(START_DATE, END_DATE)
    base = np.array([0.0115, 0.0160, 0.0215, 0.0270, 0.0300, 0.0315, 0.0320, 0.0310, 0.0335, 0.0325])
    event_shock = np.array([-0.0015, -0.0012, -0.0024, -0.0028, -0.0030, -0.0026, -0.0024, -0.0022, -0.0017, -0.0014])
    rows = []
    for idx, date in enumerate(dates):
        days_from_start = idx / max(len(dates) - 1, 1)
        trend = 0.0025 * days_from_start
        wave = 0.00035 * np.sin(idx / 2.0 + MATURITIES / 6.0)
        event_weight = 1.0 if date.strftime("%Y-%m-%d") >= POST_EVENT_DATE else 0.0
        zero_rates = base + trend + wave + event_weight * event_shock
        ytm_rates = zero_rates + 0.0004 * np.cos(MATURITIES / 4.0)
        for maturity, zero_rate, ytm_rate in zip(MATURITIES, zero_rates, ytm_rates):
            rows.append(
                {
                    "date": date.strftime("%Y-%m-%d"),
                    "treasno": f"FALLBACK-{date.strftime('%Y%m%d')}-{maturity_label(float(maturity))}",
                    "crspid": f"FALLBACK-{maturity_label(float(maturity))}",
                    "maturity_years": float(maturity),
                    "coupon_rate": float(max(ytm_rate, 0.0)),
                    "clean_price": 100.0,
                    "accrued_interest": 0.0,
                    "dirty_price": 100.0,
                    "bid": 99.98,
                    "ask": 100.02,
                    "bid_ask_spread": 0.04,
                    "tdyld": float(ytm_rate),
                    "duration": float(maturity * 365.25),
                    "price_source": "fallback",
                    "price_flag": "synthetic",
                    "tips_index_ratio": np.nan,
                    "frequency": 2.0,
                    "used_in_curve": False,
                    "exclusion_reason": "",
                    "fitted_zero_rate": float(zero_rate),
                }
            )
    meta = {
        "source": "fallback",
        "source_detail": "deterministic fallback data",
        "tdyld_annualization": 1.0,
        "excluded_tips_count": 0,
        "invalid_price_count": 0,
    }
    return pd.DataFrame(rows), meta


def select_curve_surfaces(input_check: pd.DataFrame) -> tuple[pd.DataFrame, pd.DataFrame, pd.DataFrame]:
    eligible = input_check[input_check["exclusion_reason"] == ""].copy()
    selected_indices: set[int] = set()
    observed_rows = []
    zero_rows = []
    missing_rows = []
    for date, group in eligible.groupby("date"):
        clean = group.sort_values("maturity_years")
        for target in MATURITIES:
            if clean.empty:
                missing_rows.append({"date": date, "maturity": float(target)})
                continue
            nearest_idx = (clean["maturity_years"] - target).abs().idxmin()
            nearest = clean.loc[nearest_idx]
            selected_indices.add(int(nearest_idx))
            observed_rows.append(
                {
                    "date": date,
                    "maturity": float(target),
                    "observed_ytm": float(nearest["tdyld"]),
                }
            )
            zero_rate = nearest["fitted_zero_rate"]
            if not np.isfinite(zero_rate):
                zero_rate = nearest["tdyld"]
            zero_rows.append(
                {
                    "date": date,
                    "maturity": float(target),
                    "zero_rate": float(zero_rate),
                }
            )

    input_check = input_check.copy()
    input_check.loc[list(selected_indices), "used_in_curve"] = True
    input_check.loc[(input_check["exclusion_reason"] == "") & (~input_check["used_in_curve"]), "exclusion_reason"] = (
        "not nearest on-the-run maturity grid point"
    )
    observed = pd.DataFrame(observed_rows).sort_values(["date", "maturity"]).reset_index(drop=True)
    zero = pd.DataFrame(zero_rows).sort_values(["date", "maturity"]).reset_index(drop=True)
    missing = pd.DataFrame(missing_rows)
    return input_check, observed, zero, missing


def write_quality_report(
    input_check: pd.DataFrame,
    zero_surface: pd.DataFrame,
    observed_surface: pd.DataFrame,
    missing: pd.DataFrame,
    meta: dict[str, object],
    comparison: pd.DataFrame | None = None,
) -> None:
    rows: list[dict[str, str]] = []

    source = str(meta.get("source", "unknown"))
    rows.append(
        {
            "check": "private data used or fallback data used",
            "severity": "info",
            "message": f"{source} data used ({meta.get('source_detail', '')}).",
        }
    )

    per_date = input_check[input_check["exclusion_reason"] == ""].groupby("date").size()
    if per_date.empty:
        rows.append({"check": "number of securities per date", "severity": "error", "message": "No eligible securities."})
    else:
        rows.append(
            {
                "check": "number of securities per date",
                "severity": "info",
                "message": f"Eligible securities per date: min={int(per_date.min())}, median={float(per_date.median()):.1f}, max={int(per_date.max())}.",
            }
        )

    if missing is not None and not missing.empty:
        rows.append(
            {
                "check": "missing maturities",
                "severity": "warning",
                "message": f"Missing maturity grid points: {len(missing)}.",
            }
        )
    else:
        rows.append({"check": "missing maturities", "severity": "pass", "message": "All maturity grid points populated."})

    if comparison is not None and not comparison.empty:
        max_gap = comparison[
            ["difference_otr_ytm_minus_gov_bp", "difference_zero_minus_gov_bp"]
        ].abs().max().max()
        severity = "warning" if max_gap > 75.0 else "pass"
        rows.append(
            {
                "check": "large gap between government par curve and on-the-run curve",
                "severity": severity,
                "message": f"Largest absolute comparison gap is {max_gap:.1f} bp.",
            }
        )
    else:
        rows.append(
            {
                "check": "large gap between government par curve and on-the-run curve",
                "severity": "info",
                "message": "Government comparison not available when this report was written.",
            }
        )

    median_zero = zero_surface["zero_rate"].abs().median()
    severity = "pass" if 0.001 <= median_zero <= 0.20 else "warning"
    rows.append(
        {
            "check": "percent vs decimal scale check",
            "severity": severity,
            "message": f"Median absolute zero rate is {median_zero:.6f}; rates are stored as decimals.",
        }
    )
    rows.append(
        {
            "check": "TIPS excluded",
            "severity": "pass",
            "message": f"TIPS-like rows excluded: {int(meta.get('excluded_tips_count', 0))}.",
        }
    )
    rows.append(
        {
            "check": "invalid prices excluded",
            "severity": "pass",
            "message": f"Rows with invalid prices excluded: {int(meta.get('invalid_price_count', 0))}.",
        }
    )
    rows.append(
        {
            "check": "whether TDYLD was used directly or only for diagnostics",
            "severity": "info",
            "message": "TDYLD is used for the observed YTM surface and diagnostics; the fitted-zero surface uses dirty-price zero-equivalent rates when private prices are available.",
        }
    )
    rows.append(
        {
            "check": "whether fitted zero curve was built from dirty prices",
            "severity": "pass" if source == "private" else "warning",
            "message": "Dirty price = TDNOMPRC + TDACCINT is used for fitted zero-equivalent rates." if source == "private" else "Fallback zero rates are deterministic because private dirty prices are unavailable.",
        }
    )
    rows.append(
        {
            "check": "whether maturities were mapped explicitly",
            "severity": "pass",
            "message": "Maturities are explicitly mapped to the fixed grid, never inferred by column position.",
        }
    )
    pd.DataFrame(rows, columns=["check", "severity", "message"]).to_csv(
        EVENT_DIR / "fomc_curve_quality_report.csv", index=False
    )


def prepare_on_the_run_curve(input_path: str | None = None) -> tuple[pd.DataFrame, pd.DataFrame, pd.DataFrame, dict[str, object]]:
    EVENT_DIR.mkdir(parents=True, exist_ok=True)
    write_column_diagnostics()
    path = resolve_input_path(input_path)
    if path is not None:
        try:
            input_check, meta = build_input_check_from_private(path)
        except Exception as exc:
            print(f"Private Treasury CSV could not be used ({exc}); using deterministic fallback data.")
            input_check, meta = build_fallback_input_check()
    else:
        input_check, meta = build_fallback_input_check()

    input_check, observed_surface, zero_surface, missing = select_curve_surfaces(input_check)
    public_check = input_check[
        [
            "date",
            "treasno",
            "crspid",
            "maturity_years",
            "coupon_rate",
            "clean_price",
            "accrued_interest",
            "dirty_price",
            "bid",
            "ask",
            "bid_ask_spread",
            "tdyld",
            "duration",
            "price_source",
            "price_flag",
            "used_in_curve",
            "exclusion_reason",
        ]
    ].copy()
    public_check.to_csv(EVENT_DIR / "on_the_run_curve_input_check.csv", index=False)
    observed_surface.to_csv(EVENT_DIR / "fomc_june2022_observed_ytm_surface.csv", index=False)
    write_quality_report(input_check, zero_surface, observed_surface, missing, meta)
    return input_check, observed_surface, zero_surface, meta


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", help="Optional path to a private CRSP/WRDS Treasury CSV")
    args = parser.parse_args()
    _, observed, zero, meta = prepare_on_the_run_curve(args.input)
    print(
        f"Prepared on-the-run curve diagnostics using {meta['source']} data: "
        f"{len(observed)} observed points, {len(zero)} fitted zero points."
    )


if __name__ == "__main__":
    main()
