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

    def load(self, qasm: str):
        import cirq
        from cirq.contrib.qasm_import import circuit_from_qasm

        return cirq.Simulator(), circuit_from_qasm(qasm)

    def simulate(self, circuit) -> None:
        sim, c = circuit
        sim.simulate(c)

    def state(self, circuit):
        import numpy as np

        sim, c = circuit
        return np.asarray(sim.simulate(c).final_state_vector, dtype=complex)
