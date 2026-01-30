#!/usr/bin/env bash
set -euo pipefail

# ShmeaDB_UnitTest.sh
# Build + run unit tests in one go (Linux/macOS). From repo root.

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${ROOT_DIR}/build"

# Defaults (override via env vars):
: "${BUILD_TYPE:=Debug}"           # Debug | Release | RelWithDebInfo | MinSizeRel
: "${GENERATOR:=}"                 # e.g. "Unix Makefiles" or "Ninja" (empty = CMake default)
: "${JOBS:=}"                      # e.g. 8 (empty = auto)
: "${CTEST_VERBOSE:=1}"            # 0/1
: "${CLEAN:=0}"                    # 0/1

if [[ "${CLEAN}" == "1" ]]; then
  echo "[UT] Cleaning build dir: ${BUILD_DIR}"
  rm -rf "${BUILD_DIR}"
fi

mkdir -p "${BUILD_DIR}"

echo "[UT] Configuring (BUILD_TYPE=${BUILD_TYPE})"
cmake_args=(
  "-S" "${ROOT_DIR}"
  "-B" "${BUILD_DIR}"
  "-DCMAKE_BUILD_TYPE=${BUILD_TYPE}"
  "-DSHMEA_BUILD_TESTS=ON"
)

if [[ -n "${GENERATOR}" ]]; then
  cmake_args+=("-G" "${GENERATOR}")
fi

cmake "${cmake_args[@]}"

echo "[UT] Building"
build_args=( "--build" "${BUILD_DIR}" )
if [[ -n "${JOBS}" ]]; then
  build_args+=( "-j" "${JOBS}" )
fi
cmake "${build_args[@]}"

# Run via CTest if tests are registered, else run the test exe directly.
echo "[UT] Running tests"

if [[ "${CTEST_VERBOSE}" == "1" ]]; then
  ctest --test-dir "${BUILD_DIR}" -V --output-on-failure || true
else
  ctest --test-dir "${BUILD_DIR}" --output-on-failure || true
fi

# If CTest says "No tests were found", try running the built exe directly.
# Detect common names and locations.
TEST_EXE=""
candidates=(
  "${BUILD_DIR}/unit-tests/shmea_unit_tests"
  "${BUILD_DIR}/unit-tests/shmea-unit-tests"
  "${BUILD_DIR}/unit-tests/shmea_unit_tests.exe"
  "${BUILD_DIR}/unit-tests/shmea-unit-tests.exe"
)

for c in "${candidates[@]}"; do
  if [[ -x "${c}" ]]; then
    TEST_EXE="${c}"
    break
  fi
done

if [[ -n "${TEST_EXE}" ]]; then
  echo "[UT] Also running test executable directly for full stdout:"
  echo "     ${TEST_EXE}"
  "${TEST_EXE}"
else
  echo "[UT] Note: Could not find a runnable test executable in expected locations."
  echo "     If ctest reported no tests, confirm unit-tests remember to add_test(...) and BUILD_TESTING is ON."
fi

echo "[UT] Done."

