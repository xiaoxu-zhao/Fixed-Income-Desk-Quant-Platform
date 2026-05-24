from pathlib import Path
import sys

import pandas as pd


def project_root() -> Path:
    return Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else Path(__file__).resolve().parents[1]


def metric_map(path: Path) -> dict:
    frame = pd.read_csv(path)
    return dict(zip(frame.iloc[:, 0], frame.iloc[:, 1]))


def money(value: float) -> str:
    sign = "-" if value < 0 else ""
    value = abs(value)
    if value >= 1_000_000:
        return f"{sign}${value / 1_000_000:.2f}mm"
    if value >= 1_000:
        return f"{sign}${value / 1_000:.1f}k"
    return f"{sign}${value:.0f}"


def bp(value: float) -> str:
    return f"{value:+.2f} bp"


def main() -> None:
    root = project_root()
    output = root / "output"
    daily = metric_map(output / "daily_report.csv")
    pnl = metric_map(output / "pnl_explain.csv")
    execution = metric_map(output / "execution_report.csv")
    keyrate = pd.read_csv(output / "keyrate_dv01.csv")
    intraday = pd.read_csv(output / "intraday_pnl.csv")

    daily_pnl = float(daily["daily_pnl"])
    full_pnl = float(pnl["full_revaluation_pnl"])
    key_explained = float(pnl["keyrate_explained"])
    residual = float(pnl["residual"])
    largest_row = keyrate.iloc[keyrate["dv01"].abs().idxmax()]
    largest_key = int(largest_row["key_maturity"])
    largest_dv01 = float(largest_row["dv01"])
    fill_ratio = float(execution.get("fill_ratio", 0.0))
    slippage_ticks = float(execution.get("slippage_ticks", 0.0))
    avg_fill = float(execution.get("average_fill_price", 0.0))
    arrival = float(execution.get("arrival_price", 0.0))
    max_intraday_loss = float(intraday["intraday_pnl"].min())
    max_intraday_gain = float(intraday["intraday_pnl"].max())
    close_intraday_pnl = float(intraday["intraday_pnl"].iloc[-1])
    max_yield_move = float(intraday["implied_yield_move_bp"].abs().max())

    direction = "positive" if daily_pnl >= 0 else "negative"
    pnl_driver = "rate rally" if daily_pnl >= 0 else "rate selloff"
    explained_share = 0.0 if full_pnl == 0 else abs(key_explained / full_pnl)
    residual_text = "modest" if abs(residual) < max(1.0, abs(full_pnl) * 0.25) else "material"
    hedge = daily.get("suggested_hedge", "review_key_rate_duration")
    execution_comment = "adverse" if slippage_ticks > 0 else "favorable"

    lines = [
        "# Fixed Income Desk Quant Daily Briefing",
        "",
        "This deterministic run simulates one working day of a front-office fixed-income desk quant supporting both voice and electronic trading workflows.",
        "",
        "## Executive Summary",
        "",
        (
            f"The portfolio finished with {direction} daily P&L of {money(daily_pnl)}. "
            f"The move was primarily consistent with a {pnl_driver}, with full-revaluation "
            f"P&L of {money(full_pnl)} and key-rate attribution explaining {explained_share:.0%} "
            f"of the move."
        ),
        "",
        "## Morning Pricing And Risk",
        "",
        (
            f"The largest key-rate exposure is the {largest_key}Y bucket with DV01 of "
            f"{money(largest_dv01)} per bp. Total desk DV01 is {daily.get('total_dv01', 'n/a')} "
            f"per bp, and the suggested hedge is `{hedge}`."
        ),
        "",
        "## End-Of-Day P&L Explain",
        "",
        (
            f"First-order key-rate attribution was {money(key_explained)}. "
            f"The residual was {money(residual)}, which is {residual_text} relative to the "
            "full revaluation move. Convexity and carry are included as simple approximations "
            "so the unexplained bucket remains visible."
        ),
        "",
        "## Midday Electronic Market Replay",
        "",
        (
            f"The synthetic ZN futures replay generated a maximum absolute implied yield shock of "
            f"{bp(max_yield_move)}. Mapping that shock through the 10Y key-rate DV01 produced an "
            f"intraday P&L range from {money(max_intraday_loss)} to {money(max_intraday_gain)}, "
            f"with final intraday mark-to-market of {money(close_intraday_pnl)}."
        ),
        "",
        "## Hedge Execution Analytics",
        "",
        (
            f"The simulated hedge execution filled {fill_ratio:.1%} of the requested quantity. "
            f"Average fill was {avg_fill:.6f} versus arrival {arrival:.6f}, with slippage of "
            f"{slippage_ticks:.2f} ticks. The execution result was {execution_comment} versus arrival "
            "under the demo sign convention."
        ),
        "",
        "## Reproducibility Note",
        "",
        "The report is generated from versioned synthetic inputs and a fixed-seed market simulator. It does not call external market-data services or LLM APIs.",
        "",
    ]
    (output / "desk_summary.md").write_text("\n".join(lines), encoding="utf-8")
    print(f"Wrote {output / 'desk_summary.md'}")


if __name__ == "__main__":
    main()
