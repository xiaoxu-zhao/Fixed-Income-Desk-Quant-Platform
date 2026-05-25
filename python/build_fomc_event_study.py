from __future__ import annotations

import argparse
from pathlib import Path

import numpy as np
import pandas as pd

from prepare_on_the_run_curve import (
    EVENT_DIR,
    FORWARD_BUCKETS,
    MATURITIES,
    PRE_EVENT_DATE,
    POST_EVENT_DATE,
    maturity_label,
    prepare_on_the_run_curve,
)


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

    input_check, observed_surface, zero_surface, meta = prepare_on_the_run_curve(args.input)
    zero_surface = zero_surface.sort_values(["date", "maturity"]).reset_index(drop=True)
    forward_surface = build_forward_surface(zero_surface)
    curve_moves = build_curve_moves(zero_surface)
    curve_factors = build_curve_factors(zero_surface)

    EVENT_DIR.mkdir(parents=True, exist_ok=True)
    zero_surface.to_csv(EVENT_DIR / "fomc_june2022_zero_surface.csv", index=False)
    observed_surface.to_csv(EVENT_DIR / "fomc_june2022_observed_ytm_surface.csv", index=False)
    forward_surface.to_csv(EVENT_DIR / "fomc_june2022_forward_surface.csv", index=False)
    curve_moves.to_csv(EVENT_DIR / "fomc_june2022_curve_moves.csv", index=False)
    curve_factors.to_csv(EVENT_DIR / "fomc_june2022_curve_factors.csv", index=False)
    print(f"Built June 2022 FOMC event-study data using {meta['source']} data ({meta['source_detail']}).")


if __name__ == "__main__":
    main()
