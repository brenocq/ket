# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
"""Adapter interface for the simulator benchmark.

To add a library: drop a ``<name>_adapter.py`` here that defines a subclass of
``Adapter`` (usually ``PythonAdapter``). The orchestrator auto-discovers it and
includes it whenever ``available()`` is true.
"""

import re
import time


class Adapter:
    """A benchmarkable quantum simulator."""

    name = "?"

    def available(self) -> bool:
        """True if this library can be benchmarked (installed / buildable)."""
        return False

    def version(self) -> str:
        """Installed version string, for the report (best-effort)."""
        return "?"

    def benchmark(self, qasm: str, reps: int) -> list:
        """Per-run seconds to simulate `qasm` to its final state (`reps` runs)."""
        raise NotImplementedError

    def statevector(self, qasm: str):
        """Final state vector as a 1-D complex numpy array, for the correctness
        cross-check. Return None if this library can't expose it."""
        return None


class PythonAdapter(Adapter):
    """Base for in-process Python adapters.

    Subclasses implement ``load`` (build the native circuit, not timed) and
    ``simulate`` (evolve to the final state vector, timed). Only the simulation
    is measured; parsing and circuit construction are excluded. ``state`` returns
    the final amplitudes for the correctness check (not timed).
    """

    def load(self, qasm: str):
        raise NotImplementedError

    def simulate(self, circuit) -> None:
        raise NotImplementedError

    def state(self, circuit):
        """Final state vector as a complex numpy array (None if unsupported)."""
        return None

    def benchmark(self, qasm: str, reps: int) -> list:
        circuit = self.load(qasm)
        self.simulate(circuit)  # warm-up run (discarded)
        times = []
        for _ in range(reps):
            start = time.perf_counter()
            self.simulate(circuit)
            times.append(time.perf_counter() - start)
        return times

    def statevector(self, qasm: str):
        return self.state(self.load(qasm))


def num_qubits(qasm: str) -> int:
    """Total qubit count from the ``qreg`` declarations in a QASM string."""
    return sum(int(n) for n in re.findall(r"qreg\s+\w+\[(\d+)\]", qasm))
