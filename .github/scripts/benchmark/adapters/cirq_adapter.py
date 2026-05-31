# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
from base import PythonAdapter


class CirqAdapter(PythonAdapter):
    name = "Cirq"

    def available(self) -> bool:
        try:
            import cirq  # noqa: F401
            from cirq.contrib.qasm_import import circuit_from_qasm  # noqa: F401

            return True
        except ImportError:
            return False

    def version(self) -> str:
        import cirq

        return cirq.__version__

    def supports(self, n: int, clifford: bool) -> bool:
        # CliffordSimulator handles any Clifford n; the dense simulator is pure
        # Python, so cap it well below the others.
        return True if clifford else n <= 24

    def load(self, qasm: str, clifford: bool):
        import cirq
        from cirq.contrib.qasm_import import circuit_from_qasm

        circuit = circuit_from_qasm(qasm)
        qubits = sorted(circuit.all_qubits())
        circuit.append(cirq.measure(*qubits, key="m"))
        sim = cirq.CliffordSimulator() if clifford else cirq.Simulator()
        return sim, circuit

    def sample_once(self, prepared) -> None:
        sim, circuit = prepared
        sim.run(circuit, repetitions=1)

    def statevector(self, qasm: str):
        import cirq
        import numpy as np
        from cirq.contrib.qasm_import import circuit_from_qasm

        result = cirq.Simulator().simulate(circuit_from_qasm(qasm))
        return np.asarray(result.final_state_vector, dtype=complex)
