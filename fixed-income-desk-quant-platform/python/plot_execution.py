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

    intraday = pd.read_csv(root / "output" / "intraday_pnl.csv")
    execution = pd.read_csv(root / "output" / "execution_report.csv")
    metrics = dict(zip(execution["metric"], execution["value"]))

    plt.figure(figsize=(10, 4))
    plt.plot(intraday["timestamp_ns"] / 1_000_000, intraday["mid_price"], color="#4C78A8")
    plt.xlabel("Time (ms)")
    plt.ylabel("ZN mid price")
    plt.title("Intraday Futures Mid Price")
    plt.grid(True, alpha=0.3)
    plt.tight_layout()
    savefig(plots / "intraday_mid_price.png")

    plt.figure(figsize=(10, 4))
    plt.plot(intraday["timestamp_ns"] / 1_000_000, intraday["spread"] * 64.0, color="#B07AA1")
    plt.xlabel("Time (ms)")
    plt.ylabel("Spread (64ths)")
    plt.title("Top-of-Book Bid-Ask Spread")
    plt.grid(True, alpha=0.3)
    plt.tight_layout()
    savefig(plots / "intraday_spread.png")

    selected = pd.DataFrame({
        "metric": ["slippage_ticks", "fill_ratio", "implementation_shortfall", "execution_cost_vs_vwap"],
        "value": [
            float(metrics.get("slippage_ticks", 0.0)),
            float(metrics.get("fill_ratio", 0.0)),
            float(metrics.get("implementation_shortfall", 0.0)),
            float(metrics.get("execution_cost_vs_vwap", 0.0)),
        ],
    })
    plt.figure(figsize=(8, 4))
    colors = ["#E15759" if x > 0 else "#59A14F" for x in selected["value"]]
    plt.bar(selected["metric"], selected["value"], color=colors)
    plt.title("Execution Slippage and Fill Metrics")
    plt.xticks(rotation=20, ha="right")
    plt.axhline(0, color="black", linewidth=0.8)
    plt.tight_layout()
    savefig(plots / "execution_report.png")


if __name__ == "__main__":
    main()
