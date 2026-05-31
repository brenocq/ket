// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
//
// Deutsch-Jozsa: decides whether f: {0,1}^3 -> {0,1} is *constant* (same value
// for every input) or *balanced* (0 on half the inputs, 1 on the other half)
// with a single oracle query -- a classical algorithm may need up to
// 2^(n-1)+1 = 5 queries.
//
// q[0..2] are the inputs, q[3] is the ancilla. The ancilla starts in |->, so
// the oracle U_f: |x>|y> -> |x>|y ^ f(x)> kicks a phase (-1)^f(x) back onto the
// inputs; a final layer of Hadamards then concentrates the inputs onto |000>
// iff f is constant. Here f(x) = x0 ^ x1 ^ x2 is balanced, so the inputs end on
// |111> -- any nonzero result means balanced.
//
//   ket-cli draw   examples/deutsch_jozsa.qasm
//   ket-cli run    examples/deutsch_jozsa.qasm
//   ket-gui        examples/deutsch_jozsa.qasm
OPENQASM 2.0;
include "qelib1.inc";

qreg q[4];
creg c[3];

// Ancilla -> |1>, then Hadamards put inputs in superposition and the ancilla
// in |->.
x q[3];
h q[0];
h q[1];
h q[2];
h q[3];
barrier q;

// Oracle U_f for f(x) = x0 ^ x1 ^ x2 (balanced).
cx q[0], q[3];
cx q[1], q[3];
cx q[2], q[3];
barrier q;

// Interfere the inputs back.
h q[0];
h q[1];
h q[2];

measure q[0] -> c[0];
measure q[1] -> c[1];
measure q[2] -> c[2];
