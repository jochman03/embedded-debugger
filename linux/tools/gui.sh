#!/usr/bin/env bash

set -euo pipefail

TOOLS_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
LINUX_DIR="$(cd -- "${TOOLS_DIR}/.." && pwd)"
EXECUTABLE="${LINUX_DIR}/build-gui/bin/debugger_gui"

if [[ ! -x "${EXECUTABLE}" ]]; then
    echo "GUI executable not found."
    echo "Run: ${TOOLS_DIR}/build_gui.sh"
    exit 1
fi

exec "${EXECUTABLE}" "$@"
