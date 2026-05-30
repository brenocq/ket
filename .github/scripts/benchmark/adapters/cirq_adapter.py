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

    def load(self, qasm: str):
        import cirq
        from cirq.contrib.qasm_import import circuit_from_qasm

        return cirq.Simulator(), circuit_from_qasm(qasm)

    def simulate(self, circuit) -> None:
        sim, c = circuit
        sim.simulate(c)
