// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
//
// Bell state: (|00> + |11>) / sqrt(2). A Hadamard on q[0] creates a
// superposition, and a CNOT entangles q[1] with it.
//
//   ket-cli draw   examples/bell.qasm
//   ket-cli run    examples/bell.qasm
//   ket-cli sample examples/bell.qasm --shots 1000
OPENQASM 2.0;
include "qelib1.inc";

qreg q[2];
creg c[2];

h q[0];
cx q[0], q[1];

measure q[0] -> c[0];
measure q[1] -> c[1];
