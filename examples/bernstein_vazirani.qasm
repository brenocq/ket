// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
//
// Bernstein-Vazirani: a hidden 3-bit string s defines an oracle f(x) = s.x
// (mod 2) -- the parity of the input bits where s has a 1. The algorithm
// recovers all bits of s with a SINGLE oracle query; a classical algorithm
// needs one query per bit.
//
// As in Deutsch-Jozsa, the inputs q[0..2] go into a uniform superposition and
// the ancilla q[3] into |->, so the oracle kicks back a phase (-1)^(s.x). A
// final layer of Hadamards decodes that phase pattern directly into the basis
// state |s>, so measuring the input register yields s. Here s = 101 (CNOTs from
// q[0] and q[2]), so the inputs collapse to |101>.
//
//   ket-cli draw   examples/bernstein_vazirani.qasm
//   ket-cli run    examples/bernstein_vazirani.qasm
//   ket-gui        examples/bernstein_vazirani.qasm
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

// Oracle U_f for s = 101: one CNOT onto the ancilla from each input bit set
// in s.
cx q[0], q[3];
cx q[2], q[3];
barrier q;

// Decode the phase pattern back into |s>.
h q[0];
h q[1];
h q[2];

measure q[0] -> c[0];
measure q[1] -> c[1];
measure q[2] -> c[2];
