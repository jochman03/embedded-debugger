#!/usr/bin/env bash

set -euo pipefail

TOOLS_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
LINUX_DIR="$(cd -- "${TOOLS_DIR}/.." && pwd)"
BUILD_DIR="${LINUX_DIR}/build-cli"
BUILD_TYPE="${BUILD_TYPE:-Debug}"

cmake \
    -S "${LINUX_DIR}" \
    -B "${BUILD_DIR}" \
    -G Ninja \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DFETCHCONTENT_BASE_DIR="${LINUX_DIR}/.deps" \
    -DBUILD_GUI=OFF \
    -DBUILD_CLI=ON

cmake --build "${BUILD_DIR}"