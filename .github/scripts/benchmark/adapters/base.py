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

    def benchmark(self, qasm: str, reps: int) -> float:
        """Average seconds to simulate `qasm` to its final state, over `reps`."""
        raise NotImplementedError


class PythonAdapter(Adapter):
    """Base for in-process Python adapters.

    Subclasses implement ``load`` (build the native circuit, not timed) and
    ``simulate`` (evolve to the final state vector, timed). Only the simulation
    is measured; parsing and circuit construction are excluded.
    """

    def load(self, qasm: str):
        raise NotImplementedError

    def simulate(self, circuit) -> None:
        raise NotImplementedError

    def benchmark(self, qasm: str, reps: int) -> float:
        circuit = self.load(qasm)
        self.simulate(circuit)  # warm-up run (discarded)
        total = 0.0
        for _ in range(reps):
            start = time.perf_counter()
            self.simulate(circuit)
            total += time.perf_counter() - start
        return total / reps


def num_qubits(qasm: str) -> int:
    """Total qubit count from the ``qreg`` declarations in a QASM string."""
    return sum(int(n) for n in re.findall(r"qreg\s+\w+\[(\d+)\]", qasm))
