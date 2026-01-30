#!/usr/bin/env bash
set -euo pipefail

# ShmeaDB_Install.sh
# Configure + build + install ShmeaDB (Linux/macOS). From repo root.

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${ROOT_DIR}/build-install"

# Defaults (override via env vars):
: "${BUILD_TYPE:=Release}"               # Release is typical for installs
: "${INSTALL_PREFIX:=${HOME}/.local}"    # override if desired
: "${GENERATOR:=}"                       # e.g. "Unix Makefiles" or "Ninja" (empty = CMake default)
: "${JOBS:=}"                            # e.g. 8 (empty = auto)
: "${CLEAN:=0}"                          # 0/1
: "${WITH_TESTS:=0}"                     # 0/1 (usually OFF for install builds)

if [[ "${CLEAN}" == "1" ]]; then
  echo "[INSTALL] Cleaning build dir: ${BUILD_DIR}"
  rm -rf "${BUILD_DIR}"
fi

mkdir -p "${BUILD_DIR}"

echo "[INSTALL] Configuring (BUILD_TYPE=${BUILD_TYPE})"
cmake_args=(
  "-S" "${ROOT_DIR}"
  "-B" "${BUILD_DIR}"
  "-DCMAKE_BUILD_TYPE=${BUILD_TYPE}"
  "-DCMAKE_INSTALL_PREFIX=${INSTALL_PREFIX}"
  "-DSHMEA_BUILD_TESTS=$([[ "${WITH_TESTS}" == "1" ]] && echo ON || echo OFF)"
)

if [[ -n "${GENERATOR}" ]]; then
  cmake_args+=("-G" "${GENERATOR}")
fi

cmake "${cmake_args[@]}"

echo "[INSTALL] Building"
build_args=( "--build" "${BUILD_DIR}" )
if [[ -n "${JOBS}" ]]; then
  build_args+=( "-j" "${JOBS}" )
fi
cmake "${build_args[@]}"

echo "[INSTALL] Installing to: ${INSTALL_PREFIX}"
cmake --install "${BUILD_DIR}"

echo "[INSTALL] Done."
echo "[INSTALL] Tip: ensure ${INSTALL_PREFIX}/lib/cmake/shmea is in your CMAKE_PREFIX_PATH if another project does find_package(shmea)."

