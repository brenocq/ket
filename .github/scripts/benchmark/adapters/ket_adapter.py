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

    def load(self, qasm: str):
        import ket

        return ket.from_qasm(qasm)

    def simulate(self, circuit) -> None:
        import ket

        ket.run(circuit)

    def state(self, circuit):
        import ket
        import numpy as np

        s = ket.run(circuit)
        return np.array([s[i] for i in range(len(s))], dtype=complex)
