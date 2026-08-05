#!/usr/bin/env bash

set -e

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

"$ROOT_DIR/tools/build.sh"
exec "$ROOT_DIR/build/serial_monitor"