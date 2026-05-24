from pathlib import Path
import sys

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import pandas as pd


def project_root() -> Path:
    return Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else Path(__file__).resolve().parents[1]


def savefig(path: Path) -> None:
    plt.savefig(path, dpi=160, metadata={"Date": None})
    plt.close()


def main() -> None:
    root = project_root()
    plots = root / "output" / "plots"
    plots.mkdir(parents=True, exist_ok=True)

    pnl = pd.read_csv(root / "output" / "pnl_explain.csv")
    attribution = pnl[pnl["component"].isin([
        "full_revaluation_pnl",
        "parallel_rates",
        "keyrate_explained",
        "curve_twist_slope",
        "convexity",
        "carry_approximation",
        "residual",
    ])].copy()
    attribution["value"] = attribution["value"].astype(float)

    plt.figure(figsize=(10, 5))
    colors = ["#59A14F" if x >= 0 else "#E15759" for x in attribution["value"]]
    plt.bar(attribution["component"], attribution["value"], color=colors)
    plt.ylabel("P&L ($)")
    plt.title("P&L Explain")
    plt.xticks(rotation=25, ha="right")
    plt.axhline(0, color="black", linewidth=0.8)
    plt.tight_layout()
    savefig(plots / "pnl_explain.png")

    intraday = pd.read_csv(root / "output" / "intraday_pnl.csv")
    plt.figure(figsize=(10, 4))
    plt.plot(intraday["timestamp_ns"] / 1_000_000, intraday["intraday_pnl"], color="#F28E2B")
    plt.xlabel("Time (ms)")
    plt.ylabel("Intraday P&L ($)")
    plt.title("Intraday Portfolio P&L From Futures-Implied Yield Shock")
    plt.grid(True, alpha=0.3)
    plt.tight_layout()
    savefig(plots / "intraday_pnl.png")


if __name__ == "__main__":
    main()
