#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

if [ ! -d build ]; then
  ./scripts/build.sh
fi

ctest --test-dir build --build-config Release --output-on-failure

