#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PRESET="${1:-Debug}"

"$ROOT_DIR/Tools/build.sh" "$PRESET"
exec "$ROOT_DIR/Tools/flash.sh" "$PRESET"