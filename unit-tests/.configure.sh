#!/usr/bin/env bash
# Legacy unit-test configure/build entry point; also supports invocation via sh.
[ -n "${BASH_VERSION:-}" ] || exec bash "$0" "$@"
set -euo pipefail
source_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
cd "$source_dir"
preset="linux-release"
cmake_args=()
for arg in "$@"; do
    case "$arg" in
        -D?*=*) cmake_args+=("$arg") ;;
        --help|-h) echo "Usage: sh .configure.sh [-DNAME=VALUE ...]"; exit 0 ;;
        -*) echo "Unknown option: $arg" >&2; exit 2 ;;
        *) echo "Unknown argument: $arg" >&2; exit 2 ;;
    esac
done
cmake --preset "$preset" "${cmake_args[@]}"
cmake --build --preset "$preset" --parallel "${CMAKE_BUILD_PARALLEL_LEVEL:-8}"
