# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
import os
import re
import shutil
import subprocess
import tempfile

from base import Adapter

_HERE = os.path.dirname(os.path.abspath(__file__))
_SRC = os.path.normpath(os.path.join(_HERE, "..", "qpp_bench"))
_BUILD = os.path.join(_SRC, "build")


def _binary() -> str:
    # CMake names it `qpp_bench` (or `qpp_bench.exe` on Windows).
    for name in ("qpp_bench", "qpp_bench.exe", os.path.join("Release", "qpp_bench.exe")):
        path = os.path.join(_BUILD, name)
        if os.path.exists(path):
            return path
    return os.path.join(_BUILD, "qpp_bench")


def _pinned_version() -> str:
    """The qpp tag pinned in the harness CMakeLists (e.g. v7.0.3)."""
    try:
        text = open(os.path.join(_SRC, "CMakeLists.txt"), encoding="utf-8").read()
        m = re.search(r"GIT_TAG\s+v?([\w.]+)", text)  # [\w.] stops at the ')'
        return m.group(1) if m else "?"
    except OSError:
        return "?"


def _run(args: list) -> str:
    with tempfile.NamedTemporaryFile("w", suffix=".qasm", delete=False) as f:
        f.write(args[0])
        path = f.name
    try:
        out = subprocess.run(
            [_binary(), path, *args[1:]], check=True, capture_output=True, text=True
        )
        return out.stdout
    finally:
        os.unlink(path)


class QppAdapter(Adapter):
    """Quantum++ has no Python API, so we compile a small C++ harness that
    loads the QASM and times the simulation itself, and shell out to it."""

    name = "Quantum++"

    def available(self) -> bool:
        if os.path.exists(_binary()):
            return True
        if shutil.which("cmake") is None:
            return False
        try:
            print("  building the Quantum++ harness (first run: fetches qpp + Eigen)...")
            subprocess.run(
                ["cmake", "-S", _SRC, "-B", _BUILD, "-DCMAKE_BUILD_TYPE=Release"],
                check=True,
                capture_output=True,
            )
            subprocess.run(
                ["cmake", "--build", _BUILD, "--config", "Release", "-j"],
                check=True,
                capture_output=True,
            )
        except subprocess.CalledProcessError as exc:
            print("  Quantum++ build failed; skipping:")
            print((exc.stderr or b"").decode()[-600:])
            return False
        return os.path.exists(_binary())

    def version(self) -> str:
        return _pinned_version()

    def benchmark(self, qasm: str, reps: int) -> list:
        return [float(t) for t in _run([qasm, str(reps)]).split()]

    def statevector(self, qasm: str):
        import numpy as np

        pairs = [line.split() for line in _run([qasm, "--state"]).splitlines() if line]
        return np.array([float(re_) + 1j * float(im) for re_, im in pairs], dtype=complex)
