from __future__ import annotations

import argparse
import os
from pathlib import Path

import numpy as np
import pandas as pd


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_PRIVATE_CSV = Path("C:/Users/xzhaox/Downloads/treasuries.csv")
EVENT_DIR = ROOT / "output" / "event_study"

START_DATE = "2022-06-01"
END_DATE = "2022-06-28"
EVENT_DATE = "2022-06-15"
PRE_EVENT_DATE = "2022-06-14"
POST_EVENT_DATE = "2022-06-16"

MATURITIES = np.array([0.25, 0.50, 1.00, 2.00, 3.00, 5.00, 7.00, 10.00, 20.00, 30.00])
FORWARD_BUCKETS = list(zip(MATURITIES[:-1], MATURITIES[1:]))


def maturity_label(years: float) -> str:
    if years < 1.0:
        return f"{int(round(years * 12))}M"
    return f"{years:g}Y"


def resolve_input_path(cli_path: str | None) -> Path | None:
    if cli_path:
        path = Path(cli_path)
        return path if path.exists() else None
    raw_env = os.environ.get("FIQ_TREASURIES_CSV")
    if raw_env:
        env_path = Path(raw_env)
        if env_path.exists():
            return env_path
    return DEFAULT_PRIVATE_CSV if DEFAULT_PRIVATE_CSV.exists() else None


def load_private_curve_data(path: Path) -> pd.DataFrame:
    columns = ["CALDT", "TMATDT", "TDYLD", "TDNOMPRC", "TDACCINT", "ITYPE"]
    frame = pd.read_csv(path, usecols=lambda col: col in columns)
    required = {"CALDT", "TMATDT", "TDYLD"}
    missing = required.difference(frame.columns)
    if missing:
        raise ValueError(f"Missing required Treasury columns: {sorted(missing)}")

    frame["CALDT"] = pd.to_datetime(frame["CALDT"], errors="coerce")
    frame["TMATDT"] = pd.to_datetime(frame["TMATDT"], errors="coerce")
    frame["TDYLD"] = pd.to_numeric(frame["TDYLD"], errors="coerce")
    if "TDNOMPRC" in frame:
        frame["TDNOMPRC"] = pd.to_numeric(frame["TDNOMPRC"], errors="coerce")

    frame = frame[
        (frame["CALDT"] >= START_DATE)
        & (frame["CALDT"] <= END_DATE)
        & frame["CALDT"].notna()
        & frame["TMATDT"].notna()
        & frame["TDYLD"].notna()
        & (frame["TDYLD"] > -0.01)
    ].copy()
    if "TDNOMPRC" in frame:
        frame = frame[(frame["TDNOMPRC"].isna()) | (frame["TDNOMPRC"] > 0.0)]

    frame["maturity"] = (frame["TMATDT"] - frame["CALDT"]).dt.days / 365.25
    frame = frame[(frame["maturity"] > 0.02) & (frame["maturity"] <= 31.0)].copy()
    if frame.empty:
        raise ValueError("No usable Treasury rows found in the event-study window")

    # CRSP TDYLD is a daily promised yield in this extract. Annualize when the
    # median is in daily-yield units; leave already annualized data unchanged.
    median_abs_yield = frame["TDYLD"].abs().median()
    annualization = 365.25 if median_abs_yield < 0.01 else 1.0
    frame["zero_rate"] = frame["TDYLD"] * annualization
    frame = frame[(frame["zero_rate"] > -0.02) & (frame["zero_rate"] < 0.20)].copy()
    return frame


def nearest_curve_by_date(frame: pd.DataFrame) -> pd.DataFrame:
    rows: list[dict[str, float | str]] = []
    for date, group in frame.groupby("CALDT"):
        clean = group.sort_values("maturity").dropna(subset=["maturity", "zero_rate"])
        if clean.empty:
            continue
        for target in MATURITIES:
            nearest_idx = (clean["maturity"] - target).abs().idxmin()
            nearest = clean.loc[nearest_idx]
            rows.append(
                {
                    "date": date.strftime("%Y-%m-%d"),
                    "maturity": float(target),
                    "zero_rate": float(nearest["zero_rate"]),
                }
            )
    surface = pd.DataFrame(rows)
    required_dates = {PRE_EVENT_DATE, POST_EVENT_DATE}
    if not required_dates.issubset(set(surface["date"])):
        raise ValueError("Private data do not contain the required pre/post event dates")
    return surface


def fallback_zero_surface() -> pd.DataFrame:
    dates = pd.bdate_range(START_DATE, END_DATE)
    base = np.array([0.0115, 0.0160, 0.0215, 0.0270, 0.0300, 0.0315, 0.0320, 0.0310, 0.0335, 0.0325])
    event_shock = np.array([-0.0015, -0.0012, -0.0024, -0.0028, -0.0030, -0.0026, -0.0024, -0.0022, -0.0017, -0.0014])
    rows: list[dict[str, float | str]] = []
    for idx, date in enumerate(dates):
        days_from_start = idx / max(len(dates) - 1, 1)
        trend = 0.0025 * days_from_start
        wave = 0.00035 * np.sin(idx / 2.0 + MATURITIES / 6.0)
        event_weight = 1.0 if date.strftime("%Y-%m-%d") >= POST_EVENT_DATE else 0.0
        rates = base + trend + wave + event_weight * event_shock
        for maturity, rate in zip(MATURITIES, rates):
            rows.append(
                {
                    "date": date.strftime("%Y-%m-%d"),
                    "maturity": float(maturity),
                    "zero_rate": float(rate),
                }
            )
    return pd.DataFrame(rows)


def build_forward_surface(zero_surface: pd.DataFrame) -> pd.DataFrame:
    rows: list[dict[str, float | str]] = []
    for date, group in zero_surface.groupby("date"):
        rates = dict(zip(group["maturity"], group["zero_rate"]))
        for start, end in FORWARD_BUCKETS:
            z_start = rates[float(start)]
            z_end = rates[float(end)]
            forward_rate = (z_end * end - z_start * start) / (end - start)
            rows.append(
                {
                    "date": date,
                    "forward_start": float(start),
                    "forward_end": float(end),
                    "bucket": f"{maturity_label(start)}-{maturity_label(end)}",
                    "forward_rate": float(forward_rate),
                }
            )
    return pd.DataFrame(rows)


def build_curve_moves(zero_surface: pd.DataFrame) -> pd.DataFrame:
    pre = zero_surface[zero_surface["date"] == PRE_EVENT_DATE].set_index("maturity")["zero_rate"]
    post = zero_surface[zero_surface["date"] == POST_EVENT_DATE].set_index("maturity")["zero_rate"]
    rows = []
    for maturity in MATURITIES:
        pre_rate = float(pre.loc[float(maturity)])
        post_rate = float(post.loc[float(maturity)])
        rows.append(
            {
                "maturity": float(maturity),
                "zero_rate_pre": pre_rate,
                "zero_rate_post": post_rate,
                "move_bp": (post_rate - pre_rate) * 10000.0,
            }
        )
    return pd.DataFrame(rows)


def build_curve_factors(zero_surface: pd.DataFrame) -> pd.DataFrame:
    rows = []
    for date, group in zero_surface.groupby("date"):
        rates = dict(zip(group["maturity"], group["zero_rate"]))
        level = np.mean([rates[2.0], rates[5.0], rates[10.0], rates[30.0]])
        slope = rates[10.0] - rates[2.0]
        curvature = 2.0 * rates[5.0] - rates[2.0] - rates[10.0]
        rows.append(
            {
                "date": date,
                "level": float(level),
                "slope_10y_2y": float(slope),
                "curvature_2s5s10s": float(curvature),
            }
        )
    return pd.DataFrame(rows).sort_values("date")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", help="Optional path to a private CRSP/WRDS Treasury CSV")
    args = parser.parse_args()

    input_path = resolve_input_path(args.input)
    if input_path is not None:
        try:
            raw = load_private_curve_data(input_path)
            zero_surface = nearest_curve_by_date(raw)
            source = f"private Treasury CSV: {input_path}"
        except Exception as exc:
            print(f"Private Treasury CSV could not be used ({exc}); using deterministic fallback data.")
            zero_surface = fallback_zero_surface()
            source = "deterministic fallback data"
    else:
        zero_surface = fallback_zero_surface()
        source = "deterministic fallback data"

    zero_surface = zero_surface.sort_values(["date", "maturity"]).reset_index(drop=True)
    forward_surface = build_forward_surface(zero_surface)
    curve_moves = build_curve_moves(zero_surface)
    curve_factors = build_curve_factors(zero_surface)

    EVENT_DIR.mkdir(parents=True, exist_ok=True)
    zero_surface.to_csv(EVENT_DIR / "fomc_june2022_zero_surface.csv", index=False)
    forward_surface.to_csv(EVENT_DIR / "fomc_june2022_forward_surface.csv", index=False)
    curve_moves.to_csv(EVENT_DIR / "fomc_june2022_curve_moves.csv", index=False)
    curve_factors.to_csv(EVENT_DIR / "fomc_june2022_curve_factors.csv", index=False)
    print(f"Built June 2022 FOMC event-study data using {source}.")


if __name__ == "__main__":
    main()
