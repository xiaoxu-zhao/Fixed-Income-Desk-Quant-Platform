#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

rm -rf build
rm -f output/*.csv output/*.md output/dashboard.html
rm -f output/plots/*.png

