// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
//
// Grover's search over 3 qubits (8 items), marking |101>. With one solution in
// 8, the optimal number of iterations is floor((pi/4)*sqrt(8)) = 2, after which
// |101> is measured with ~94% probability.
//
//   ket-cli draw   examples/grover.qasm
//   ket-cli sample examples/grover.qasm --shots 1000
//   ket-gui        examples/grover.qasm
//
// The oracle phase-flips |101> (X on q1 maps it to |111>, ccx+H is a
// multi-controlled Z, X on q1 maps back). The diffusion operator reflects
// about the uniform superposition. Both blocks repeat twice, with barriers
// separating each stage.
OPENQASM 2.0;
include "qelib1.inc";

qreg q[3];
creg c[3];

// Uniform superposition.
h q[0];
h q[1];
h q[2];
barrier q;

// ---- Iteration 1 ----
// Oracle: phase-flip |101>.
x q[1];
h q[2];
ccx q[0], q[1], q[2];
h q[2];
x q[1];
barrier q;
// Diffusion.
h q[0];
h q[1];
h q[2];
x q[0];
x q[1];
x q[2];
h q[2];
ccx q[0], q[1], q[2];
h q[2];
x q[0];
x q[1];
x q[2];
h q[0];
h q[1];
h q[2];
barrier q;

// ---- Iteration 2 ----
// Oracle: phase-flip |101>.
x q[1];
h q[2];
ccx q[0], q[1], q[2];
h q[2];
x q[1];
barrier q;
// Diffusion.
h q[0];
h q[1];
h q[2];
x q[0];
x q[1];
x q[2];
h q[2];
ccx q[0], q[1], q[2];
h q[2];
x q[0];
x q[1];
x q[2];
h q[0];
h q[1];
h q[2];
barrier q;

measure q[0] -> c[0];
measure q[1] -> c[1];
measure q[2] -> c[2];

