#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

BUILD_DIR="${FIQ_BUILD_DIR:-build}"

if [ ! -d "$BUILD_DIR" ]; then
  ./scripts/build.sh
fi

ctest --test-dir "$BUILD_DIR" --build-config Release --output-on-failure
