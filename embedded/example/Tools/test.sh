#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PRESET="${1:-Test}"

cd "$ROOT_DIR"

cmake --preset "$PRESET"
cmake --build --preset "$PRESET"
ctest --preset "$PRESET" --output-on-failure