// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
//
// GHZ state: the three-qubit maximally entangled state,
// (|000> + |111>) / sqrt(2). A Hadamard on q[0] creates a superposition, then a
// chain of CNOTs spreads it across q[1] and q[2] — measuring any one qubit
// instantly determines all three. In the debugger, each qubit's Bloch vector
// collapses to the centre (|r| = 0) as the entanglement forms.
//
//   ket-cli draw   examples/ghz.qasm
//   ket-cli sample examples/ghz.qasm --shots 1000
//   ket-gui        examples/ghz.qasm
OPENQASM 2.0;
include "qelib1.inc";

qreg q[3];
creg c[3];

h q[0];
cx q[0], q[1];
cx q[1], q[2];

measure q[0] -> c[0];
measure q[1] -> c[1];
measure q[2] -> c[2];
