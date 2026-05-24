#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

BUILD_DIR="${FIQ_BUILD_DIR:-build}"

rm -f output/*.csv output/*.md output/dashboard.html
rm -f output/plots/*.png
mkdir -p output/plots data/market_events

find_exe() {
  local name="$1"
  for candidate in \
    "${BUILD_DIR}/${name}" \
    "${BUILD_DIR}/${name}.exe" \
    "${BUILD_DIR}/Release/${name}" \
    "${BUILD_DIR}/Release/${name}.exe" \
    "${BUILD_DIR}/Debug/${name}" \
    "${BUILD_DIR}/Debug/${name}.exe"; do
    if [ -x "$candidate" ]; then
      printf '%s\n' "$candidate"
      return 0
    fi
  done
  echo "Could not find executable: ${name}" >&2
  return 1
}

detect_python() {
  if command -v python3 >/dev/null 2>&1 && python3 --version >/dev/null 2>&1; then
    PYTHON_CMD=(python3)
    return 0
  fi
  if command -v python >/dev/null 2>&1 && python --version >/dev/null 2>&1; then
    PYTHON_CMD=(python)
    return 0
  fi
  if command -v py >/dev/null 2>&1 && py -3 --version >/dev/null 2>&1; then
    PYTHON_CMD=(py -3)
    return 0
  fi
  echo "Could not find a working Python command" >&2
  return 1
}

./scripts/build.sh
./scripts/run_tests.sh

"$(find_exe run_daily_desk_report)" .
"$(find_exe run_intraday_simulation)" .
"$(find_exe run_full_desk_day)" .

detect_python
PYTHONHASHSEED=0 "${PYTHON_CMD[@]}" python/plot_curves.py .
PYTHONHASHSEED=0 "${PYTHON_CMD[@]}" python/plot_risk.py .
PYTHONHASHSEED=0 "${PYTHON_CMD[@]}" python/plot_pnl.py .
PYTHONHASHSEED=0 "${PYTHON_CMD[@]}" python/plot_execution.py .
PYTHONHASHSEED=0 "${PYTHON_CMD[@]}" python/generate_desk_summary.py .
PYTHONHASHSEED=0 "${PYTHON_CMD[@]}" python/dashboard.py --static

echo "Full demo complete. Outputs are in $ROOT/output"
