from __future__ import annotations

from pathlib import Path
import warnings

import matplotlib

matplotlib.use("Agg")

import matplotlib.dates as mdates
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd


ROOT = Path(__file__).resolve().parents[1]
EVENT_DIR = ROOT / "output" / "event_study"
PLOTS_DIR = ROOT / "output" / "plots"

PRE_EVENT_DATE = "2022-06-14"
POST_EVENT_DATE = "2022-06-16"


def load_event_data() -> tuple[pd.DataFrame, pd.DataFrame, pd.DataFrame, pd.DataFrame]:
    zero = pd.read_csv(EVENT_DIR / "fomc_june2022_zero_surface.csv")
    forward = pd.read_csv(EVENT_DIR / "fomc_june2022_forward_surface.csv")
    moves = pd.read_csv(EVENT_DIR / "fomc_june2022_curve_moves.csv")
    factors = pd.read_csv(EVENT_DIR / "fomc_june2022_curve_factors.csv")
    zero["date"] = pd.to_datetime(zero["date"])
    forward["date"] = pd.to_datetime(forward["date"])
    factors["date"] = pd.to_datetime(factors["date"])
    return zero, forward, moves, factors


def savefig(name: str) -> None:
    PLOTS_DIR.mkdir(parents=True, exist_ok=True)
    with warnings.catch_warnings():
        warnings.simplefilter("ignore", UserWarning)
        plt.tight_layout()
    plt.savefig(PLOTS_DIR / name, dpi=170, bbox_inches="tight")
    plt.close()


def plot_pre_post(zero: pd.DataFrame) -> None:
    fig, ax = plt.subplots(figsize=(9.5, 5.4))
    for date, label, color in [
        (PRE_EVENT_DATE, "Pre-event: 2022-06-14", "#0f766e"),
        (POST_EVENT_DATE, "Post-event: 2022-06-16", "#9a3412"),
    ]:
        curve = zero[zero["date"] == pd.Timestamp(date)].sort_values("maturity")
        ax.plot(curve["maturity"], curve["zero_rate"] * 100.0, marker="o", linewidth=2.2, label=label, color=color)
    ax.set_title("June 2022 FOMC Treasury Zero/Yield Curve: Pre vs Post")
    ax.set_xlabel("Maturity (years)")
    ax.set_ylabel("Zero/yield proxy (%)")
    ax.legend()
    ax.grid(True, alpha=0.3)
    savefig("fomc_zero_curve_pre_post.png")


def plot_curve_move(moves: pd.DataFrame) -> None:
    fig, ax = plt.subplots(figsize=(9.5, 5.4))
    colors = np.where(moves["move_bp"] >= 0.0, "#9a3412", "#0f766e")
    ax.bar(moves["maturity"].astype(str), moves["move_bp"], color=colors)
    ax.axhline(0.0, color="#1d252c", linewidth=1.0)
    ax.set_title("Post-Event Curve Move by Maturity")
    ax.set_xlabel("Maturity (years)")
    ax.set_ylabel("Move from 2022-06-14 to 2022-06-16 (bp)")
    ax.grid(axis="y", alpha=0.3)
    savefig("fomc_curve_move_bp.png")


def surface_axis_dates(ax, dates: pd.Series) -> np.ndarray:
    unique_dates = pd.Series(sorted(pd.to_datetime(dates).unique()))
    x = np.arange(len(unique_dates))
    step = max(1, len(unique_dates) // 6)
    ticks = x[::step]
    ax.set_xticks(ticks)
    ax.set_xticklabels([unique_dates.iloc[i].strftime("%m-%d") for i in ticks], rotation=20, ha="right")
    return x


def plot_zero_surface(zero: pd.DataFrame) -> None:
    pivot = zero.pivot(index="date", columns="maturity", values="zero_rate").sort_index()
    date_idx = surface_axis_dates(plt.figure().add_subplot(111, projection="3d"), pivot.index)
    plt.close()

    maturities = pivot.columns.to_numpy(dtype=float)
    x_grid, y_grid = np.meshgrid(date_idx, maturities)
    z_grid = pivot.to_numpy().T * 100.0

    fig = plt.figure(figsize=(10, 6.6))
    ax = fig.add_subplot(111, projection="3d")
    ax.plot_surface(x_grid, y_grid, z_grid, cmap="viridis", edgecolor="none", alpha=0.92)
    surface_axis_dates(ax, pivot.index)
    ax.set_title("June 2022 Treasury Zero/Yield Curve Surface")
    ax.set_xlabel("Observation date")
    ax.set_ylabel("Maturity (years)")
    ax.set_zlabel("Zero/yield proxy (%)")
    ax.view_init(elev=26, azim=-130)
    savefig("fomc_zero_rate_surface_3d.png")


def plot_forward_surface(forward: pd.DataFrame) -> None:
    pivot = forward.pivot(index="date", columns="forward_start", values="forward_rate").sort_index()
    date_idx = surface_axis_dates(plt.figure().add_subplot(111, projection="3d"), pivot.index)
    plt.close()

    starts = pivot.columns.to_numpy(dtype=float)
    x_grid, y_grid = np.meshgrid(date_idx, starts)
    z_grid = pivot.to_numpy().T * 100.0

    fig = plt.figure(figsize=(10, 6.6))
    ax = fig.add_subplot(111, projection="3d")
    ax.plot_surface(x_grid, y_grid, z_grid, cmap="plasma", edgecolor="none", alpha=0.92)
    surface_axis_dates(ax, pivot.index)
    ax.set_title("June 2022 Implied Forward-Rate Surface")
    ax.set_xlabel("Observation date")
    ax.set_ylabel("Forward start (years)")
    ax.set_zlabel("Forward rate (%)")
    ax.view_init(elev=26, azim=-130)
    savefig("fomc_forward_rate_surface_3d.png")


def plot_curve_factors(factors: pd.DataFrame) -> None:
    fig, ax_level = plt.subplots(figsize=(10, 5.8))
    ax_shape = ax_level.twinx()

    ax_level.plot(factors["date"], factors["level"] * 100.0, marker="o", color="#0f766e", label="Level")
    ax_shape.plot(factors["date"], factors["slope_10y_2y"] * 10000.0, marker="s", color="#1d4ed8", label="10Y-2Y slope")
    ax_shape.plot(
        factors["date"],
        factors["curvature_2s5s10s"] * 10000.0,
        marker="^",
        color="#9a3412",
        label="2s5s10s curvature",
    )

    ax_level.axvline(pd.Timestamp("2022-06-15"), color="#6b7280", linestyle="--", linewidth=1.2)
    ax_level.set_title("Level, Slope, and Curvature Around June 2022 FOMC")
    ax_level.set_xlabel("Date")
    ax_level.set_ylabel("Level (%)")
    ax_shape.set_ylabel("Slope / curvature (bp)")
    ax_level.xaxis.set_major_formatter(mdates.DateFormatter("%m-%d"))
    ax_level.grid(True, alpha=0.3)

    handles_1, labels_1 = ax_level.get_legend_handles_labels()
    handles_2, labels_2 = ax_shape.get_legend_handles_labels()
    ax_level.legend(handles_1 + handles_2, labels_1 + labels_2, loc="best")
    savefig("fomc_level_slope_curvature.png")


def main() -> None:
    zero, forward, moves, factors = load_event_data()
    plot_pre_post(zero)
    plot_curve_move(moves)
    plot_zero_surface(zero)
    plot_forward_surface(forward)
    plot_curve_factors(factors)
    print(f"Wrote FOMC event-study plots to {PLOTS_DIR}")


if __name__ == "__main__":
    main()
