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

    instrument = pd.read_csv(root / "output" / "instrument_pv.csv")
    keyrate = pd.read_csv(root / "output" / "keyrate_dv01.csv")

    plt.figure(figsize=(10, 5))
    colors = ["#59A14F" if x >= 0 else "#E15759" for x in instrument["pv_today"]]
    plt.bar(instrument["instrument_id"], instrument["pv_today"] / 1_000_000, color=colors)
    plt.ylabel("PV today ($mm)")
    plt.title("Instrument-Level PV")
    plt.xticks(rotation=20)
    plt.tight_layout()
    savefig(plots / "instrument_pv.png")

    plt.figure(figsize=(8, 4))
    colors = ["#59A14F" if x >= 0 else "#E15759" for x in keyrate["dv01"]]
    plt.bar(keyrate["key_maturity"].astype(str) + "Y", keyrate["dv01"], color=colors)
    plt.ylabel("DV01 ($ per bp)")
    plt.title("Key-Rate DV01")
    plt.axhline(0, color="black", linewidth=0.8)
    plt.tight_layout()
    savefig(plots / "keyrate_dv01.png")


if __name__ == "__main__":
    main()
