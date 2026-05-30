# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
"""Ket: a quantum computing library.

Build circuits, simulate them on a state-vector backend, and sample
measurements.

    >>> import ket
    >>> c = ket.Circuit(2)
    >>> c.h(0)
    >>> c.cnot(0, 1)
    >>> state = ket.run(c)
    >>> outcome = ket.measure(state, seed=42)
"""

from ._ket import Circuit, StateVector, measure, run, sample

__all__ = ["Circuit", "StateVector", "measure", "run", "sample"]
