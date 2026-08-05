#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PRESET="${1:-Debug}"

BUILD_DIR="$ROOT_DIR/build/$PRESET"
OPENOCD_CONFIG="${2:-$BUILD_DIR/openocd.cfg}"

if ! command -v openocd >/dev/null 2>&1; then
    echo "Error: openocd not found in PATH." >&2
    exit 1
fi

if [ ! -d "$BUILD_DIR" ]; then
    echo "Error: build directory does not exist: $BUILD_DIR" >&2
    echo "Run tools/build.sh $PRESET first." >&2
    exit 1
fi

if [ ! -f "$OPENOCD_CONFIG" ]; then
    echo "Error: OpenOCD configuration not found: $OPENOCD_CONFIG" >&2
    echo "Run tools/build.sh $PRESET first." >&2
    exit 1
fi

mapfile -t ELF_FILES < <(
    find "$BUILD_DIR" -type f -name '*.elf' -print
)

if [ "${#ELF_FILES[@]}" -eq 0 ]; then
    echo "Error: no ELF file found in $BUILD_DIR." >&2
    exit 1
fi

if [ "${#ELF_FILES[@]}" -gt 1 ]; then
    echo "Error: more than one ELF file found:" >&2
    printf '  %s\n' "${ELF_FILES[@]}" >&2
    exit 1
fi

ELF_FILE="${ELF_FILES[0]}"

echo "Flashing: $ELF_FILE"
echo "OpenOCD config: $OPENOCD_CONFIG"

openocd \
    -f "$OPENOCD_CONFIG" \
    -c "program {$ELF_FILE} verify reset exit"

echo "Flash completed successfully."