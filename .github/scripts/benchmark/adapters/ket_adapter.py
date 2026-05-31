# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
from base import PythonAdapter


class KetAdapter(PythonAdapter):
    name = "Ket"

    def available(self) -> bool:
        try:
            import ket  # noqa: F401

            return True
        except ImportError:
            return False

    def version(self) -> str:
        import importlib.metadata

        return importlib.metadata.version("ket")

    def supports(self, n: int, clifford: bool) -> bool:
        return True if clifford else n <= 28  # stabilizer handles any Clifford n

    def load(self, qasm: str, clifford: bool):
        import os

        import ket

        # Match the run's thread budget (the harness sets OMP_NUM_THREADS).
        ket.set_num_threads(int(os.environ.get("OMP_NUM_THREADS", "1")))
        circuit = ket.from_qasm(qasm)
        circuit.measure_all()
        return circuit

    def sample_once(self, circuit) -> None:
        import ket

        # Auto-selects the stabilizer backend for Clifford circuits.
        ket.sample(circuit)

    def statevector(self, qasm: str):
        import ket
        import numpy as np

        s = ket.run(ket.from_qasm(qasm))
        return np.array([s[i] for i in range(len(s))], dtype=complex)
