from pathlib import Path
import argparse
import html
import os

import pandas as pd


ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "output"
PLOTS = OUTPUT / "plots"


def read_csv(name: str) -> pd.DataFrame:
    return pd.read_csv(OUTPUT / name)


def render_static_html() -> None:
    sections = []
    for title, file_name in [
        ("Market Overview", "daily_report.csv"),
        ("Portfolio Valuation", "instrument_pv.csv"),
        ("Key-Rate DV01", "keyrate_dv01.csv"),
        ("P&L Explain", "pnl_explain.csv"),
        ("Execution Analytics", "execution_report.csv"),
    ]:
        frame = read_csv(file_name)
        sections.append(f"<h2>{html.escape(title)}</h2>{frame.to_html(index=False)}")

    images = [
        "yield_curves.png",
        "curve_move_bp.png",
        "instrument_pv.png",
        "keyrate_dv01.png",
        "pnl_explain.png",
        "intraday_mid_price.png",
        "intraday_spread.png",
        "intraday_pnl.png",
        "execution_report.png",
    ]
    image_html = "\n".join(
        f'<figure><img src="plots/{name}" alt="{html.escape(name)}"><figcaption>{html.escape(name)}</figcaption></figure>'
        for name in images
        if (PLOTS / name).exists()
    )
    summary = (OUTPUT / "desk_summary.md").read_text(encoding="utf-8") if (OUTPUT / "desk_summary.md").exists() else ""
    page = f"""<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <title>Fixed Income Desk Quant Dashboard</title>
  <style>
    body {{ font-family: Arial, sans-serif; margin: 32px; color: #1f2933; }}
    h1 {{ margin-bottom: 4px; }}
    h2 {{ margin-top: 32px; }}
    table {{ border-collapse: collapse; margin: 12px 0 24px; min-width: 480px; }}
    th, td {{ border: 1px solid #d7dee8; padding: 6px 9px; text-align: right; }}
    th:first-child, td:first-child {{ text-align: left; }}
    img {{ max-width: 900px; width: 100%; border: 1px solid #d7dee8; }}
    figure {{ margin: 24px 0; }}
    pre {{ white-space: pre-wrap; background: #f5f7fa; padding: 16px; }}
  </style>
</head>
<body>
  <h1>Fixed Income Desk Quant Dashboard</h1>
  <p>One deterministic desk-day simulation covering voice pricing/risk support, electronic market replay, hedge execution analytics, and end-of-day reporting.</p>
  {''.join(sections)}
  <h2>Plots</h2>
  {image_html}
  <h2>Desk Summary</h2>
  <pre>{html.escape(summary)}</pre>
</body>
</html>
"""
    (OUTPUT / "dashboard.html").write_text(page, encoding="utf-8")
    print(f"Wrote {OUTPUT / 'dashboard.html'}")


def render_streamlit() -> None:
    import streamlit as st

    st.set_page_config(page_title="Fixed Income Desk Quant Dashboard", layout="wide")
    st.title("Fixed Income Desk Quant Dashboard")
    st.subheader("Market Overview")
    st.dataframe(read_csv("daily_report.csv"), use_container_width=True)

    c1, c2 = st.columns(2)
    with c1:
        st.subheader("Portfolio Valuation")
        st.dataframe(read_csv("instrument_pv.csv"), use_container_width=True)
        st.image(str(PLOTS / "instrument_pv.png"))
    with c2:
        st.subheader("Key-Rate DV01")
        st.dataframe(read_csv("keyrate_dv01.csv"), use_container_width=True)
        st.image(str(PLOTS / "keyrate_dv01.png"))

    st.subheader("P&L Explain")
    st.dataframe(read_csv("pnl_explain.csv"), use_container_width=True)
    st.image(str(PLOTS / "pnl_explain.png"))

    st.subheader("Intraday Market Replay")
    st.image(str(PLOTS / "intraday_mid_price.png"))
    st.image(str(PLOTS / "intraday_spread.png"))
    st.image(str(PLOTS / "intraday_pnl.png"))

    st.subheader("Execution Analytics")
    st.dataframe(read_csv("execution_report.csv"), use_container_width=True)
    st.image(str(PLOTS / "execution_report.png"))

    if (OUTPUT / "desk_summary.md").exists():
        st.subheader("Desk Summary")
        st.markdown((OUTPUT / "desk_summary.md").read_text(encoding="utf-8"))


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--static", action="store_true", help="Generate output/dashboard.html instead of Streamlit UI")
    args = parser.parse_args()
    if args.static or os.environ.get("FIQ_STATIC_DASHBOARD") == "1":
        render_static_html()
        return
    try:
        render_streamlit()
    except ModuleNotFoundError:
        render_static_html()


if __name__ == "__main__":
    main()
