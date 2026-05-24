#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

BUILD_DIR="${FIQ_BUILD_DIR:-build}"

rm -rf "$BUILD_DIR"
rm -f output/*.csv output/*.md output/dashboard.html
rm -f output/plots/*.png
