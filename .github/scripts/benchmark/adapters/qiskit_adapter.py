# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
import os

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

    def version(self) -> str:
        import qiskit
        import qiskit_aer

        return f"{qiskit.__version__}/aer {qiskit_aer.__version__}"

    def supports(self, n: int, clifford: bool) -> bool:
        return True if clifford else n <= 28  # 'automatic' picks the stabilizer

    def load(self, qasm: str, clifford: bool):
        from qiskit import QuantumCircuit
        from qiskit_aer import AerSimulator

        qc = QuantumCircuit.from_qasm_str(qasm)
        qc.measure_all()
        threads = int(os.environ.get("OMP_NUM_THREADS", "1"))
        # 'automatic' selects the stabilizer method for Clifford circuits and a
        # state vector otherwise — Aer's own best-method choice. We skip transpile
        # so Aer runs the circuit directly (transpiling to its target caps at ~30
        # qubits, which a 64-qubit stabilizer circuit blows past).
        sim = AerSimulator(method="automatic", max_parallel_threads=threads)
        return sim, qc

    def sample_once(self, prepared) -> None:
        sim, qc = prepared
        sim.run(qc, shots=1).result()

    def statevector(self, qasm: str):
        import numpy as np
        from qiskit import QuantumCircuit, transpile
        from qiskit_aer import AerSimulator

        qc = QuantumCircuit.from_qasm_str(qasm)
        qc.save_statevector()
        sim = AerSimulator(method="statevector", max_parallel_threads=1)
        result = sim.run(transpile(qc, sim)).result()
        return np.asarray(result.get_statevector(), dtype=complex)
