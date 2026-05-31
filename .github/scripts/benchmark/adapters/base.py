# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
"""Adapter interface for the simulator benchmark.

The timed task is "evolve the circuit and draw one measurement sample", and each
adapter is free to use whatever method is fastest for the circuit — a stabilizer
engine for Clifford circuits, a state vector otherwise — either auto-detected by
the library or selected from the `clifford` hint the harness passes in.

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

    def supports(self, n: int, clifford: bool) -> bool:
        """Whether this library can handle an n-qubit circuit of this kind.
        A library with no stabilizer engine can't reach the large Clifford
        circuits (a 2^n state vector won't fit), so it declares the limit here."""
        return n <= 28

    def benchmark(self, qasm: str, reps: int, clifford: bool) -> list:
        """Per-run seconds to evolve `qasm` and sample one shot, `reps` times,
        using this library's best method for the circuit."""
        raise NotImplementedError

    def statevector(self, qasm: str):
        """Final state vector as a 1-D complex numpy array, for the correctness
        cross-check. Return None if this library can't expose it."""
        return None


class PythonAdapter(Adapter):
    """Base for in-process Python adapters.

    Subclasses implement ``load`` (build the best simulator + measured circuit,
    not timed) and ``sample_once`` (evolve and draw one shot, timed).
    """

    def load(self, qasm: str, clifford: bool):
        raise NotImplementedError

    def sample_once(self, prepared) -> None:
        raise NotImplementedError

    def benchmark(self, qasm: str, reps: int, clifford: bool) -> list:
        prepared = self.load(qasm, clifford)
        self.sample_once(prepared)  # warm-up run (discarded)
        times = []
        for _ in range(reps):
            start = time.perf_counter()
            self.sample_once(prepared)
            times.append(time.perf_counter() - start)
        return times


def num_qubits(qasm: str) -> int:
    """Total qubit count from the ``qreg`` declarations in a QASM string."""
    return sum(int(n) for n in re.findall(r"qreg\s+\w+\[(\d+)\]", qasm))
