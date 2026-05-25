from __future__ import annotations

from pathlib import Path

import matplotlib

matplotlib.use("Agg")

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd


ROOT = Path(__file__).resolve().parents[1]
EVENT_DIR = ROOT / "output" / "event_study"
PLOTS_DIR = ROOT / "output" / "plots"
PUBLIC_CURVE = ROOT / "data" / "public" / "treasury_par_yield_curve_june2022.csv"

MAPPING = {
    "1 Mo": 0.0833,
    "2 Mo": 0.1667,
    "3 Mo": 0.25,
    "4 Mo": 0.3333,
    "6 Mo": 0.5,
    "1 Yr": 1.0,
    "2 Yr": 2.0,
    "3 Yr": 3.0,
    "5 Yr": 5.0,
    "7 Yr": 7.0,
    "10 Yr": 10.0,
    "20 Yr": 20.0,
    "30 Yr": 30.0,
}

def load_government_curve() -> pd.DataFrame:
    if not PUBLIC_CURVE.exists():
        return pd.DataFrame(columns=["date", "maturity", "government_par_yield"])
    raw = pd.read_csv(PUBLIC_CURVE)
    raw["date"] = pd.to_datetime(raw["date"]).dt.strftime("%Y-%m-%d")
    rows = []
    for label, maturity in MAPPING.items():
        if label not in raw.columns:
            continue
        values = pd.to_numeric(raw[label], errors="coerce")
        for date, value in zip(raw["date"], values):
            if pd.notna(value):
                rows.append(
                    {
                        "date": date,
                        "maturity": float(maturity),
                        "government_par_yield": float(value) / 100.0,
                    }
                )
    return pd.DataFrame(rows)


def load_on_the_run_curve() -> tuple[pd.DataFrame, pd.DataFrame]:
    zero = pd.read_csv(EVENT_DIR / "fomc_june2022_zero_surface.csv")
    zero = zero.rename(columns={"zero_rate": "on_the_run_fitted_zero"})
    if (EVENT_DIR / "fomc_june2022_observed_ytm_surface.csv").exists():
        observed = pd.read_csv(EVENT_DIR / "fomc_june2022_observed_ytm_surface.csv")
    else:
        observed = zero.rename(columns={"on_the_run_fitted_zero": "observed_ytm"})[
            ["date", "maturity", "observed_ytm"]
        ]
    observed = observed.rename(columns={"observed_ytm": "on_the_run_observed_ytm"})
    return observed, zero


def update_quality_report(comparison: pd.DataFrame) -> None:
    report_path = EVENT_DIR / "fomc_curve_quality_report.csv"
    if not report_path.exists():
        return
    report = pd.read_csv(report_path)
    comparable = comparison.dropna(subset=["government_par_yield"])
    if comparable.empty:
        max_gap = np.nan
    else:
        max_gap = comparable[
            ["difference_otr_ytm_minus_gov_bp", "difference_zero_minus_gov_bp"]
        ].abs().max().max()
    if pd.isna(max_gap):
        severity = "info"
        message = "No public government par-yield file was provided; comparison plot uses quotation-derived curves only."
    else:
        severity = "warning" if max_gap > 75.0 else "pass"
        message = f"Largest absolute comparison gap is {max_gap:.1f} bp."
    mask = report["check"] == "large gap between government par curve and on-the-run curve"
    if mask.any():
        report.loc[mask, "severity"] = severity
        report.loc[mask, "message"] = message
    else:
        report = pd.concat(
            [
                report,
                pd.DataFrame(
                    [
                        {
                            "check": "large gap between government par curve and on-the-run curve",
                            "severity": severity,
                            "message": message,
                        }
                    ]
                ),
            ],
            ignore_index=True,
        )
    report.to_csv(report_path, index=False)


def build_comparison() -> pd.DataFrame:
    government = load_government_curve()
    observed, zero = load_on_the_run_curve()
    comparison = observed.merge(zero, on=["date", "maturity"], how="outer").merge(
        government, on=["date", "maturity"], how="left"
    )
    comparison["difference_otr_ytm_minus_gov_bp"] = (
        comparison["on_the_run_observed_ytm"] - comparison["government_par_yield"]
    ) * 10000.0
    comparison["difference_zero_minus_gov_bp"] = (
        comparison["on_the_run_fitted_zero"] - comparison["government_par_yield"]
    ) * 10000.0
    comparison = comparison[
        [
            "date",
            "maturity",
            "government_par_yield",
            "on_the_run_observed_ytm",
            "on_the_run_fitted_zero",
            "difference_otr_ytm_minus_gov_bp",
            "difference_zero_minus_gov_bp",
        ]
    ].sort_values(["date", "maturity"])
    EVENT_DIR.mkdir(parents=True, exist_ok=True)
    comparison.to_csv(EVENT_DIR / "government_vs_on_the_run_curve_comparison.csv", index=False)
    update_quality_report(comparison)
    return comparison


def plot_comparison(comparison: pd.DataFrame) -> None:
    PLOTS_DIR.mkdir(parents=True, exist_ok=True)
    fig, axes = plt.subplots(1, 2, figsize=(13.5, 5.5), sharey=True)
    for ax, date in zip(axes, ["2022-06-14", "2022-06-16"]):
        frame = comparison[comparison["date"] == date].sort_values("maturity")
        gov = frame.dropna(subset=["government_par_yield"])
        if not gov.empty:
            ax.plot(
                gov["maturity"],
                gov["government_par_yield"] * 100.0,
                marker="o",
                label="Government par yield",
                color="#1d4ed8",
            )
        ax.plot(
            frame["maturity"],
            frame["on_the_run_observed_ytm"] * 100.0,
            marker="s",
            label="On-the-run observed YTM",
            color="#0f766e",
        )
        ax.plot(
            frame["maturity"],
            frame["on_the_run_fitted_zero"] * 100.0,
            marker="^",
            label="Fitted zero curve",
            color="#9a3412",
        )
        ax.set_title(date)
        ax.set_xlabel("Maturity (years)")
        ax.grid(True, alpha=0.3)
    axes[0].set_ylabel("Rate (%)")
    handles, labels = axes[0].get_legend_handles_labels()
    fig.legend(handles, labels, loc="upper center", ncol=3, frameon=False)
    fig.suptitle("On-the-Run Observed YTM and Fitted Zero Curve Diagnostics", y=1.03)
    fig.tight_layout()
    plt.savefig(PLOTS_DIR / "government_vs_on_the_run_curve_comparison.png", dpi=170, bbox_inches="tight")
    plt.close(fig)


def main() -> None:
    comparison = build_comparison()
    plot_comparison(comparison)
    print("Wrote government/on-the-run curve comparison CSV and PNG.")


if __name__ == "__main__":
    main()
