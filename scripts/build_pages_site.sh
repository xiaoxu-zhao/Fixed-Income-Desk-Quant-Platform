#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DOCS_DIR="$ROOT/docs"
DEMO_DIR="$DOCS_DIR/demo"
ASSET_PLOTS_DIR="$DOCS_DIR/assets/plots"

mkdir -p "$DOCS_DIR/assets" "$DOCS_DIR/methodology" "$DEMO_DIR" "$ASSET_PLOTS_DIR"

rm -f "$DEMO_DIR/dashboard.html" "$DEMO_DIR"/*.csv "$DEMO_DIR"/*.md
rm -rf "$DEMO_DIR/plots"
mkdir -p "$DEMO_DIR/plots"

cp "$ROOT/output/dashboard.html" "$DEMO_DIR/dashboard.html"
cp "$ROOT/output/desk_summary.md" "$DEMO_DIR/"
cp "$ROOT/output"/*.csv "$DEMO_DIR/"
cp -R "$ROOT/output/plots/." "$DEMO_DIR/plots/"

python python/build_fomc_event_study.py
python python/plot_fomc_event_study.py
if [ -f python/compare_on_the_run_to_government_curve.py ]; then
  if ! python python/compare_on_the_run_to_government_curve.py; then
    echo "Warning: government/on-the-run curve comparison could not be built" >&2
  fi
fi

for plot in \
  fomc_zero_curve_pre_post.png \
  fomc_curve_move_bp.png \
  fomc_zero_rate_surface_3d.png \
  fomc_forward_rate_surface_3d.png \
  fomc_level_slope_curvature.png \
  government_vs_on_the_run_curve_comparison.png; do
  if [ -f "$ROOT/output/plots/$plot" ]; then
    cp "$ROOT/output/plots/$plot" "$ASSET_PLOTS_DIR/$plot"
  else
    echo "Warning: plot $plot is unavailable and will not be copied to Pages assets" >&2
  fi
done

touch "$DOCS_DIR/.nojekyll"
echo "Built GitHub Pages site under $DOCS_DIR"
