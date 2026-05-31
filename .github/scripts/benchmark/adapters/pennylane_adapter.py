# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
from base import PythonAdapter, num_qubits


def _has_clifford_device() -> bool:
    try:
        import stim  # noqa: F401  (default.clifford is a stim wrapper)

        return True
    except ImportError:
        return False


class PennyLaneAdapter(PythonAdapter):
    name = "PennyLane"

    def available(self) -> bool:
        try:
            import pennylane  # noqa: F401
            import pennylane_lightning  # noqa: F401

            return True
        except ImportError:
            return False

    def version(self) -> str:
        import importlib.metadata

        pl = importlib.metadata.version("pennylane")
        lightning = importlib.metadata.version("pennylane-lightning")
        return f"{pl}/lightning {lightning}"

    def supports(self, n: int, clifford: bool) -> bool:
        if clifford:
            return _has_clifford_device()  # default.clifford (stim) handles any n
        return n <= 28

    def load(self, qasm: str, clifford: bool):
        import pennylane as qml

        n = num_qubits(qasm)
        loaded = qml.from_qasm(qasm)
        # default.clifford (stim) for Clifford circuits, lightning otherwise.
        name = "default.clifford" if clifford else "lightning.qubit"
        dev = qml.device(name, wires=n, shots=1)

        @qml.qnode(dev)
        def circuit():
            loaded()
            return qml.sample(wires=range(n))

        return circuit

    def sample_once(self, circuit) -> None:
        circuit()

    def statevector(self, qasm: str):
        import numpy as np
        import pennylane as qml

        n = num_qubits(qasm)
        loaded = qml.from_qasm(qasm)
        dev = qml.device("lightning.qubit", wires=n)

        @qml.qnode(dev)
        def circuit():
            loaded()
            return qml.state()

        return np.asarray(circuit(), dtype=complex)
