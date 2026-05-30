# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
import os
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

    def benchmark(self, qasm: str, reps: int) -> float:
        with tempfile.NamedTemporaryFile("w", suffix=".qasm", delete=False) as f:
            f.write(qasm)
            path = f.name
        try:
            out = subprocess.run(
                [_binary(), path, str(reps)],
                check=True,
                capture_output=True,
                text=True,
            )
            return float(out.stdout.strip())
        finally:
            os.unlink(path)
