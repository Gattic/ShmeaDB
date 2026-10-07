#!/usr/bin/env bash
# Linux counterpart to build-and-run.bat; test saves stay in unit-tests/.
set -euo pipefail
source_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
cd "$source_dir"
preset="linux-release"
cmake_args=()
test_args=()
for arg in "$@"; do
    case "$arg" in
        -D?*=*) cmake_args+=("$arg") ;;
        --help|-h) echo "Usage: bash build-and-run.sh [test-selector] [-DNAME=VALUE ...]"; exit 0 ;;
        -*) echo "Unknown option: $arg" >&2; exit 2 ;;
        *) test_args+=("$arg") ;;
    esac
done
cmake --preset "$preset" "${cmake_args[@]}"
cmake --build --preset "$preset" --parallel "${CMAKE_BUILD_PARALLEL_LEVEL:-8}"
exec ./build/shmea-unit-tests "${test_args[@]}"
