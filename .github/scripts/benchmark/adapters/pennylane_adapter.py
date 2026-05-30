# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
from base import PythonAdapter, num_qubits


class PennyLaneAdapter(PythonAdapter):
    name = "PennyLane (lightning)"

    def available(self) -> bool:
        try:
            import pennylane  # noqa: F401
            import pennylane_lightning  # noqa: F401

            return True
        except ImportError:
            return False

    def load(self, qasm: str):
        import pennylane as qml

        n = num_qubits(qasm)
        loaded = qml.from_qasm(qasm)
        dev = qml.device("lightning.qubit", wires=n)

        @qml.qnode(dev)
        def circuit():
            loaded()
            return qml.state()

        return circuit

    def simulate(self, circuit) -> None:
        circuit()
