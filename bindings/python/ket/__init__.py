# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
"""Ket: a quantum computing library.

Build circuits, simulate them on a state-vector backend, and sample
measurements.

    >>> import ket
    >>> c = ket.Circuit(2)
    >>> c.h(0)
    >>> c.cx(0, 1)
    >>> state = ket.run(c)
    >>> outcome = ket.measure(state, seed=42)
"""

from ._ket import (
    Circuit,
    ProbeRun,
    State,
    measure,
    run,
    run_with_probes,
    sample,
)

__all__ = [
    "Circuit",
    "ProbeRun",
    "State",
    "measure",
    "run",
    "run_with_probes",
    "sample",
]
