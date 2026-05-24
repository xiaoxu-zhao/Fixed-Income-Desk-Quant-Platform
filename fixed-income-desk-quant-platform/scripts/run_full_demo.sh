#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

rm -f output/*.csv output/*.md output/dashboard.html
rm -f output/plots/*.png
mkdir -p output/plots data/market_events

find_exe() {
  local name="$1"
  for candidate in \
    "build/${name}" \
    "build/${name}.exe" \
    "build/Release/${name}" \
    "build/Release/${name}.exe" \
    "build/Debug/${name}" \
    "build/Debug/${name}.exe"; do
    if [ -x "$candidate" ]; then
      printf '%s\n' "$candidate"
      return 0
    fi
  done
  echo "Could not find executable: ${name}" >&2
  return 1
}

python_cmd() {
  if command -v python3 >/dev/null 2>&1; then
    printf '%s\n' python3
  else
    printf '%s\n' python
  fi
}

./scripts/build.sh
./scripts/run_tests.sh

"$(find_exe run_daily_desk_report)" .
"$(find_exe run_intraday_simulation)" .
"$(find_exe run_full_desk_day)" .

PYTHON="$(python_cmd)"
PYTHONHASHSEED=0 "$PYTHON" python/plot_curves.py .
PYTHONHASHSEED=0 "$PYTHON" python/plot_risk.py .
PYTHONHASHSEED=0 "$PYTHON" python/plot_pnl.py .
PYTHONHASHSEED=0 "$PYTHON" python/plot_execution.py .
PYTHONHASHSEED=0 "$PYTHON" python/generate_desk_summary.py .
PYTHONHASHSEED=0 "$PYTHON" python/dashboard.py --static

echo "Full demo complete. Outputs are in $ROOT/output"
