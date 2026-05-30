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

    def load(self, qasm: str):
        import ket

        return ket.from_qasm(qasm)

    def simulate(self, circuit) -> None:
        import ket

        ket.run(circuit)
