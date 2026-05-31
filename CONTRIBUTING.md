# Contributing to Ket

Thanks for your interest in Ket! Bug reports, feature ideas, and pull requests
are all welcome.

- **Found a bug or have an idea?** Open an [issue](https://github.com/brenocq/ket/issues).
- **Want to discuss something first?** Start a thread in
  [Discussions](https://github.com/brenocq/ket/discussions), or open a draft PR.
- **Ready to change code?** See below.

## Prerequisites

- A **C++20** compiler (GCC, Clang, or MSVC) and **CMake ≥ 3.20**.
- **Python ≥ 3.10** if you touch the bindings.
- For the GUI on Linux: the X11/OpenGL dev headers
  (`sudo apt-get install xorg-dev libgl1-mesa-dev`).

## Build and test

`build.sh` wraps CMake; `--help` lists every option.

```sh
./build.sh                 # build the library, examples, CLI, and GUI
./build.sh --cpp-tests     # build and run the C++ tests (GoogleTest + ctest)
./build.sh --py-tests      # build the bindings and run the Python tests (pytest)
```

Under the hood the C++ tests are GoogleTest binaries discovered by `ctest`, and
the Python tests live in [`tests/python`](tests/python). Both suites run in CI on
Linux, macOS, and Windows, so please make sure they pass locally before opening a
PR. New behavior should come with tests.

## Formatting

All C++, CMake, and Python sources are auto-formatted, and CI rejects unformatted
code. Run the formatters before committing:

```sh
./build.sh --format        # clang-format + cmake-format + ruff, in place
```

CI pins the versions so everyone agrees — install the same ones with:

```sh
pip install "clang-format~=21.1" "cmakelang==0.6.13" "ruff==0.15.15"
```

The C++ style is Google-based (see [`.clang-format`](.clang-format)); please don't
reformat unrelated lines.

## Pull requests

- Keep PRs **focused** — one logical change per PR is easiest to review.
- Branch off `main`; CI (the three OS workflows) must be green.
- Update the relevant docs (`README.md`, the benchmark README, etc.) when behavior
  changes.

### Commit messages

Ket uses [Conventional Commits](https://www.conventionalcommits.org): a
`type: imperative summary` subject, optionally followed by a blank line and a body
explaining the *why*. Common types:

- `feat:` a new feature
- `fix:` a bug fix
- `docs:` documentation only
- `refactor:` a change that neither fixes a bug nor adds a feature
- `style:` formatting / whitespace
- `test:` adding or fixing tests
- `chore:` build, tooling, CI

Example:

```
feat: fuse same-qubit gates in the state-vector backend

Merge consecutive gates on the same qubit(s) into one combined gate so a
deep circuit makes fewer sweeps over the 2^n state vector.
```

## Project layout

A quick map (the [README](README.md) has the bigger picture):

- `include/ket/`, `src/` — the C++ library (core in `src/`, public headers in
  `include/ket/`, simulation backends in `src/backends/`).
- `bindings/` — the OpenQASM parser and the pybind11 Python module.
- `tests/` — C++ (`*.cpp`) and Python (`tests/python/`) suites.
- `cli/`, `gui/`, `examples/` — the `ket-cli` tool, `ket-gui` viewer, and example
  programs.
- `.github/scripts/benchmark/` — the cross-library benchmark.

## License

By contributing, you agree that your contributions are licensed under the
project's [MIT License](LICENSE).
