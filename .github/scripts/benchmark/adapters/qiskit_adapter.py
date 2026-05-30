# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
from base import PythonAdapter


class QiskitAdapter(PythonAdapter):
    name = "Qiskit (Aer)"

    def available(self) -> bool:
        try:
            import qiskit  # noqa: F401
            from qiskit_aer import AerSimulator  # noqa: F401

            return True
        except ImportError:
            return False

    def load(self, qasm: str):
        from qiskit import QuantumCircuit, transpile
        from qiskit_aer import AerSimulator

        qc = QuantumCircuit.from_qasm_str(qasm)
        qc.save_statevector()  # capture the final state (no measurements here)
        sim = AerSimulator(method="statevector", max_parallel_threads=1)
        return sim, transpile(qc, sim)

    def simulate(self, circuit) -> None:
        sim, compiled = circuit
        sim.run(compiled).result()
