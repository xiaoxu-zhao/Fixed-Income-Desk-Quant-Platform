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

    yday = pd.read_csv(root / "data" / "curves" / "curve_yesterday.csv")
    today = pd.read_csv(root / "data" / "curves" / "curve_today.csv")
    merged = yday.merge(today, on="maturity", suffixes=("_yesterday", "_today"))
    merged["move_bp"] = (merged["zero_rate_today"] - merged["zero_rate_yesterday"]) * 10000.0

    plt.figure(figsize=(9, 5))
    plt.plot(yday["maturity"], yday["zero_rate"] * 100, marker="o", label="Yesterday")
    plt.plot(today["maturity"], today["zero_rate"] * 100, marker="o", label="Today")
    plt.xlabel("Maturity (years)")
    plt.ylabel("Zero rate (%)")
    plt.title("Treasury Zero Curve")
    plt.grid(True, alpha=0.3)
    plt.legend()
    plt.tight_layout()
    savefig(plots / "yield_curves.png")

    plt.figure(figsize=(9, 4))
    plt.bar(merged["maturity"].astype(str), merged["move_bp"], color="#4C78A8")
    plt.xlabel("Maturity (years)")
    plt.ylabel("Move (bp)")
    plt.title("Curve Move")
    plt.axhline(0, color="black", linewidth=0.8)
    plt.tight_layout()
    savefig(plots / "curve_move_bp.png")


if __name__ == "__main__":
    main()
