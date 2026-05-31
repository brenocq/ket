# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
"""Parameterized OpenQASM 2.0 circuit generators.

Small circuits (2-3 qubits) only measure per-call overhead; the kernel cost is
trivial. These generators build circuits at any width so the benchmark can reach
the regime where the 2ⁿ state-vector update — not Python dispatch — dominates.

Every generator emits gates from the portable qelib1 subset (``h``, ``rx``/
``ry``/``rz``, ``cx``, ``cu1``, ``swap``) so all libraries parse the same text.
"""

import math
import random


def _angle(x: float) -> str:
    return f"{x:.10g}"


def _header(n: int) -> list:
    return ["OPENQASM 2.0;", 'include "qelib1.inc";', f"qreg q[{n}];"]


def ghz(n: int) -> str:
    """An n-qubit GHZ state: H then a chain of CNOTs. Cheap and linear — one
    pass over the amplitudes — so it isolates memory bandwidth from gate count."""
    lines = _header(n) + ["h q[0];"]
    lines += [f"cx q[{i}],q[{i + 1}];" for i in range(n - 1)]
    return "\n".join(lines) + "\n"


def qft(n: int) -> str:
    """The quantum Fourier transform: dense controlled-phase rotations. A classic
    kernel stress test — O(n²) two-qubit gates, each touching every amplitude."""
    lines = _header(n)
    for j in range(n):
        lines.append(f"h q[{j}];")
        for k in range(j + 1, n):
            lines.append(f"cu1({_angle(math.pi / 2 ** (k - j))}) q[{k}],q[{j}];")
    for i in range(n // 2):
        lines.append(f"swap q[{i}],q[{n - 1 - i}];")
    return "\n".join(lines) + "\n"


def random_circuit(n: int, depth: int, seed: int) -> str:
    """A brickwork random circuit: per layer, a random single-qubit rotation on
    every qubit followed by a staggered layer of CNOTs. The standard throughput
    benchmark, and — being non-symmetric — a strong correctness discriminator.

    Deterministic in ``seed`` so every library runs the identical circuit."""
    rng = random.Random(seed)
    lines = _header(n)
    for layer in range(depth):
        for q in range(n):
            lines.append(f"ry({_angle(rng.uniform(0, 2 * math.pi))}) q[{q}];")
            lines.append(f"rz({_angle(rng.uniform(0, 2 * math.pi))}) q[{q}];")
        for a in range(layer % 2, n - 1, 2):
            lines.append(f"cx q[{a}],q[{a + 1}];")
    return "\n".join(lines) + "\n"


def bell() -> str:
    """The 2-qubit Bell state — kept as a small-circuit reference point that
    shows the per-call-overhead regime alongside the large circuits."""
    return "\n".join(_header(2) + ["h q[0];", "cx q[0],q[1];"]) + "\n"
