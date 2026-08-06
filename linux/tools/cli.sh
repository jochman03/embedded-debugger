#!/usr/bin/env bash

set -euo pipefail

TOOLS_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
LINUX_DIR="$(cd -- "${TOOLS_DIR}/.." && pwd)"
EXECUTABLE="${LINUX_DIR}/build-cli/bin/debugger_cli"

if [[ ! -x "${EXECUTABLE}" ]]; then
    echo "CLI executable not found."
    echo "Run: ${TOOLS_DIR}/build_cli.sh"
    exit 1
fi

exec "${EXECUTABLE}" "$@"
