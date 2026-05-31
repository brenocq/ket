// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
//
// Quantum Fourier Transform on 3 qubits. The QFT maps a basis state |j> to
//   (1/sqrt(N)) * sum_k exp(2*pi*i * j*k / N) |k>,   N = 2^3 = 8,
// using a ladder of Hadamards and controlled-phase rotations (pi/2, pi/4), then
// a swap to undo the bit reversal.
//
// The input here is |001> (j = 1), so the 8 amplitudes come out equal in
// magnitude (1/sqrt(8)) with phases sweeping a full turn in 45-degree steps --
// a rainbow across the phase-colored State panel. All measurement outcomes are
// equally likely; the phases are the point, so inspect them with `run` or in
// the GUI's State panel.
//
//   ket-cli run examples/qft.qasm
//   ket-gui     examples/qft.qasm
OPENQASM 2.0;
include "qelib1.inc";

qreg q[3];
creg c[3];

// Input |001> (j = 1).
x q[0];
barrier q;

// QFT: Hadamard on each qubit, with controlled-phase rotations from the
// lower-order qubits.
h q[2];
cp(pi/2) q[1], q[2];
cp(pi/4) q[0], q[2];
h q[1];
cp(pi/2) q[0], q[1];
h q[0];

// Reverse the qubit order.
swap q[0], q[2];

measure q[0] -> c[0];
measure q[1] -> c[1];
measure q[2] -> c[2];
