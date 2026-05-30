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

# Echoes "sudo" if writing to the given directory needs elevation, else nothing.
install_sudo() {
  local dir="$1"
  if [[ ${EUID:-$(id -u)} -eq 0 ]]; then return 0; fi
  if [[ -w "$dir" ]] || { [[ ! -e "$dir" ]] && [[ -w "$(dirname "$dir")" ]]; }; then
    return 0
  fi
  echo sudo
}

BUILD_DIR=build
BINDIR=/usr/local/bin           # ket-cli / ket-gui install location (on PATH)
SHAREDIR=/usr/local/share/ket   # ket-gui font install location
KET_PYTHON=OFF
KET_TESTS=OFF
KET_EXAMPLES=OFF
KET_CLI=OFF
KET_GUI=OFF
RUN_CPP_TESTS=0
RUN_PY_TESTS=0
CLEAN=0
FORMAT=0
INSTALL=0
UNINSTALL=0
EXPLICIT_TARGETS=0  # set when any build/test flag selects specific components

usage() {
  cat <<EOF
${BOLD}Usage:${RESET} ${CYAN}./build.sh${RESET} [options]
${DIM}With no flags, builds everything (library, examples, CLI, GUI). Pass flags to
build a focused subset, run tests, or build the Python bindings.${RESET}

${BOLD}${BLUE}Build options${RESET} (map to CMake options):
  ${GREEN}-p,  --python${RESET}         Build only the Python bindings    ${DIM}(-DKET_PYTHON=ON)${RESET}
  ${GREEN}-t,  --tests${RESET}          Build only the C++ tests          ${DIM}(-DKET_TESTS=ON)${RESET}
  ${GREEN}-e,  --examples${RESET}       Build only the example programs   ${DIM}(-DKET_EXAMPLES=ON)${RESET}
  ${GREEN}-c,  --cli${RESET}            Build only the ket-cli executable ${DIM}(-DKET_CLI=ON)${RESET}
  ${GREEN}-g,  --gui${RESET}            Build only the ket-gui executable ${DIM}(-DKET_GUI=ON)${RESET}

${BOLD}${BLUE}Test options${RESET} (build, then run):
  ${GREEN}-ct, --cpp-tests${RESET}      Run the C++ tests with ctest  ${DIM}(implies --tests)${RESET}
  ${GREEN}-pt, --py-tests${RESET}       Run the Python tests with pytest ${DIM}(implies --python)${RESET}

${BOLD}${BLUE}Other${RESET}:
  ${GREEN}-f,  --format${RESET}         Format sources (clang-format, cmake-format, ruff) and exit
  ${GREEN}-i,  --install${RESET}        Build and install ket under /usr/local (lib, headers, tools)
  ${GREEN}-u,  --uninstall${RESET}      Remove the installed ket files
       ${GREEN}--clean${RESET}          Remove the build directory before configuring
  ${GREEN}-B,  --build-dir DIR${RESET}  Build directory ${DIM}(default: build)${RESET}
  ${GREEN}-h,  --help${RESET}           Show this help

${BOLD}${BLUE}Examples${RESET}:
  ${CYAN}./build.sh${RESET}              ${DIM}# build everything (library, examples, CLI, GUI)${RESET}
  ${CYAN}./build.sh --cli${RESET}        ${DIM}# build only the CLI  (./build/cli/ket-cli)${RESET}
  ${CYAN}./build.sh --gui${RESET}        ${DIM}# build only the GUI  (./build/gui/ket-gui)${RESET}
  ${CYAN}./build.sh --examples${RESET}   ${DIM}# build only the example programs${RESET}
  ${CYAN}./build.sh --cpp-tests${RESET}  ${DIM}# build and run the C++ tests${RESET}
  ${CYAN}./build.sh --py-tests${RESET}   ${DIM}# build the bindings and run the Python tests${RESET}
  ${CYAN}./build.sh --clean${RESET}      ${DIM}# remove the build dir, then build everything${RESET}
  ${CYAN}./build.sh --install${RESET}    ${DIM}# build, then install ket-cli and ket-gui${RESET}
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    -p|--python)     KET_PYTHON=ON; EXPLICIT_TARGETS=1 ;;
    -t|--tests)      KET_TESTS=ON; EXPLICIT_TARGETS=1 ;;
    -e|--examples)   KET_EXAMPLES=ON; EXPLICIT_TARGETS=1 ;;
    -c|--cli)        KET_CLI=ON; EXPLICIT_TARGETS=1 ;;
    -g|--gui)        KET_GUI=ON; EXPLICIT_TARGETS=1 ;;
    -ct|--cpp-tests) RUN_CPP_TESTS=1; EXPLICIT_TARGETS=1 ;;
    -pt|--py-tests)  RUN_PY_TESTS=1; EXPLICIT_TARGETS=1 ;;
    -f|--format)     FORMAT=1 ;;
    -i|--install)    INSTALL=1; KET_CLI=ON; KET_GUI=ON; EXPLICIT_TARGETS=1 ;;
    -u|--uninstall)  UNINSTALL=1 ;;
    --clean)         CLEAN=1 ;;
    -B|--build-dir)  shift; BUILD_DIR="${1:?--build-dir requires an argument}" ;;
    -h|--help)       usage; exit 0 ;;
    *) err "unknown option '$1'"; usage >&2; exit 1 ;;
  esac
  shift
done

if [[ $FORMAT == 1 ]]; then
  if command -v clang-format >/dev/null 2>&1; then
    step "clang-format (C++)"
    find include src bindings examples tests cli gui \
      \( -name '*.cpp' -o -name '*.hpp' \) -print0 | xargs -0 clang-format -i
  else
    err "clang-format not found, skipping C++"
  fi

  if command -v cmake-format >/dev/null 2>&1; then
    step "cmake-format (CMake)"
    cmake-format -i CMakeLists.txt tests/CMakeLists.txt examples/CMakeLists.txt \
      cli/CMakeLists.txt gui/CMakeLists.txt
  else
    err "cmake-format not found, skipping CMake"
  fi

  RUFF=""
  if [[ -x .venv/bin/ruff ]]; then
    RUFF=.venv/bin/ruff
  elif command -v ruff >/dev/null 2>&1; then
    RUFF=ruff
  fi
  if [[ -n "$RUFF" ]]; then
    step "ruff (Python)"
    "$RUFF" format bindings/python tests/python
    "$RUFF" check --select I --fix bindings/python tests/python || true
  else
    err "ruff not found (pip install -e \".[dev]\"), skipping Python"
  fi

  ok "Formatted"
  exit 0
fi

if [[ $UNINSTALL == 1 ]]; then
  SUDO=$(install_sudo "$BINDIR")
  step "Uninstalling ket"
  # Remove exactly what was installed (if the manifest is still around)...
  if [[ -f "$BUILD_DIR/install_manifest.txt" ]]; then
    $SUDO xargs rm -f <"$BUILD_DIR/install_manifest.txt"
  fi
  # ...and tidy up the package directories / a possible older user install.
  $SUDO rm -rf "$SHAREDIR" /usr/local/include/ket /usr/local/lib/cmake/ket
  $SUDO rm -f "$BINDIR/ket-cli" "$BINDIR/ket-gui" \
    /usr/local/lib/libket.a /usr/local/lib64/libket.a
  rm -f "$HOME/.local/bin/ket-cli" "$HOME/.local/bin/ket-gui"
  hash -r 2>/dev/null || true
  ok "Uninstalled"
  exit 0
fi

# With no build/test flags, build everything that needs no extra setup: the
# library, examples, CLI, and GUI. (Python bindings and tests stay opt-in.)
if [[ $EXPLICIT_TARGETS == 0 ]]; then
  KET_EXAMPLES=ON
  KET_CLI=ON
  KET_GUI=ON
fi

# Running a suite forces the relevant part of the build on.
[[ $RUN_CPP_TESTS == 1 ]] && KET_TESTS=ON
[[ $RUN_PY_TESTS == 1 ]] && KET_PYTHON=ON

if [[ $CLEAN == 1 ]]; then
  step "Removing $BUILD_DIR"
  rm -rf "$BUILD_DIR"
fi

step "Configuring (KET_PYTHON=$KET_PYTHON, KET_TESTS=$KET_TESTS, KET_EXAMPLES=$KET_EXAMPLES, KET_CLI=$KET_CLI, KET_GUI=$KET_GUI)"
cmake -S . -B "$BUILD_DIR" \
  -DKET_PYTHON="$KET_PYTHON" \
  -DKET_TESTS="$KET_TESTS" \
  -DKET_EXAMPLES="$KET_EXAMPLES" \
  -DKET_CLI="$KET_CLI" \
  -DKET_GUI="$KET_GUI"

step "Building"
cmake --build "$BUILD_DIR" -j

if [[ $RUN_CPP_TESTS == 1 ]]; then
  step "Running C++ tests"
  ctest --test-dir "$BUILD_DIR" --output-on-failure
fi

if [[ $RUN_PY_TESTS == 1 ]]; then
  step "Running Python tests"
  # Make the freshly built extension importable as part of the `ket` package.
  cp "$BUILD_DIR"/_ket*.so bindings/python/ket/
  PYTHONPATH=bindings/python python3 -m pytest tests/python
fi

if [[ $KET_EXAMPLES == ON ]]; then
  step "Examples built — run them from $BUILD_DIR/examples/ (e.g. ./$BUILD_DIR/examples/bell)"
fi

if [[ $KET_CLI == ON ]]; then
  step "CLI built — run ./$BUILD_DIR/cli/ket-cli --help"
fi

if [[ $KET_GUI == ON ]]; then
  step "GUI built — run ./$BUILD_DIR/gui/ket-gui examples/bell.qasm"
fi

if [[ $INSTALL == 1 ]]; then
  SUDO=$(install_sudo "$BINDIR")
  if [[ -n "$SUDO" ]] && ! command -v sudo >/dev/null 2>&1; then
    err "installing under /usr/local needs root; re-run as root"
    exit 1
  fi
  step "Installing under /usr/local"
  $SUDO cmake --install "$BUILD_DIR"
  ok "Installed ket — library, headers, ket-cli, and ket-gui"
fi

ok "Done"
