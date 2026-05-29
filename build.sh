#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
#
# Convenience wrapper around CMake for building Ket and running its tests.
set -euo pipefail

cd "$(dirname "$0")"

# Colors, only when writing to a terminal (disabled when piped/redirected).
if [[ -t 1 ]]; then
  BOLD=$'\033[1m'; BLUE=$'\033[34m'; GREEN=$'\033[32m'
  RED=$'\033[31m'; CYAN=$'\033[36m'; DIM=$'\033[2m'; RESET=$'\033[0m'
else
  BOLD=''; BLUE=''; GREEN=''; RED=''; CYAN=''; DIM=''; RESET=''
fi

step() { printf '%s==>%s %s%s%s\n' "$BOLD$BLUE" "$RESET" "$BOLD" "$*" "$RESET"; }
err()  { printf '%serror:%s %s\n' "$BOLD$RED" "$RESET" "$*" >&2; }
ok()   { printf '%s==>%s %s%s%s\n' "$BOLD$GREEN" "$RESET" "$BOLD" "$*" "$RESET"; }

BUILD_DIR=build
KET_PYTHON=OFF
KET_TESTS=OFF
KET_EXAMPLES=OFF
RUN_CPP_TESTS=0
RUN_PY_TESTS=0
CLEAN=0

usage() {
  cat <<EOF
${BOLD}Usage:${RESET} ${CYAN}./build.sh${RESET} [options]

${BOLD}${BLUE}Build options${RESET} (map to CMake options):
  ${GREEN}-p,  --python${RESET}         Build the Python bindings    ${DIM}(-DKET_PYTHON=ON)${RESET}
  ${GREEN}-t,  --tests${RESET}          Build the C++ tests          ${DIM}(-DKET_TESTS=ON)${RESET}
  ${GREEN}-e,  --examples${RESET}       Build the example programs   ${DIM}(-DKET_EXAMPLES=ON)${RESET}

${BOLD}${BLUE}Test options${RESET} (build, then run):
  ${GREEN}-ct, --cpp-tests${RESET}      Run the C++ tests with ctest  ${DIM}(implies --tests)${RESET}
  ${GREEN}-pt, --py-tests${RESET}       Run the Python tests with pytest ${DIM}(implies --python)${RESET}

${BOLD}${BLUE}Other${RESET}:
       ${GREEN}--clean${RESET}          Remove the build directory before configuring
  ${GREEN}-B,  --build-dir DIR${RESET}  Build directory ${DIM}(default: build)${RESET}
  ${GREEN}-h,  --help${RESET}           Show this help

${BOLD}${BLUE}Examples${RESET}:
  ${CYAN}./build.sh${RESET}                 ${DIM}# configure + build (C++ library only)${RESET}
  ${CYAN}./build.sh -ct${RESET}             ${DIM}# build and run the C++ tests${RESET}
  ${CYAN}./build.sh -p${RESET}              ${DIM}# build with the Python bindings${RESET}
  ${CYAN}./build.sh -pt${RESET}             ${DIM}# build bindings and run the Python tests${RESET}
  ${CYAN}./build.sh -e${RESET}              ${DIM}# build the examples (./build/examples/bell)${RESET}
  ${CYAN}./build.sh --clean -ct -pt${RESET} ${DIM}# clean rebuild, run both test suites${RESET}
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    -p|--python)     KET_PYTHON=ON ;;
    -t|--tests)      KET_TESTS=ON ;;
    -e|--examples)   KET_EXAMPLES=ON ;;
    -ct|--cpp-tests) RUN_CPP_TESTS=1 ;;
    -pt|--py-tests)  RUN_PY_TESTS=1 ;;
    --clean)         CLEAN=1 ;;
    -B|--build-dir)  shift; BUILD_DIR="${1:?--build-dir requires an argument}" ;;
    -h|--help)       usage; exit 0 ;;
    *) err "unknown option '$1'"; usage >&2; exit 1 ;;
  esac
  shift
done

# Running a suite forces the relevant part of the build on.
[[ $RUN_CPP_TESTS == 1 ]] && KET_TESTS=ON
[[ $RUN_PY_TESTS == 1 ]] && KET_PYTHON=ON

if [[ $CLEAN == 1 ]]; then
  step "Removing $BUILD_DIR"
  rm -rf "$BUILD_DIR"
fi

step "Configuring (KET_PYTHON=$KET_PYTHON, KET_TESTS=$KET_TESTS, KET_EXAMPLES=$KET_EXAMPLES)"
cmake -S . -B "$BUILD_DIR" \
  -DKET_PYTHON="$KET_PYTHON" \
  -DKET_TESTS="$KET_TESTS" \
  -DKET_EXAMPLES="$KET_EXAMPLES"

step "Building"
cmake --build "$BUILD_DIR" -j

if [[ $RUN_CPP_TESTS == 1 ]]; then
  step "Running C++ tests"
  ctest --test-dir "$BUILD_DIR" --output-on-failure
fi

if [[ $RUN_PY_TESTS == 1 ]]; then
  step "Running Python tests"
  # Make the freshly built extension importable as part of the `ket` package.
  cp "$BUILD_DIR"/_ket*.so python/ket/
  PYTHONPATH=python python3 -m pytest tests/python
fi

if [[ $KET_EXAMPLES == ON ]]; then
  step "Examples built — run them from $BUILD_DIR/examples/ (e.g. ./$BUILD_DIR/examples/bell)"
fi

ok "Done"
