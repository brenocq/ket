// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
#pragma once

#include <string>

#include <ket/state.hpp>

namespace ket {

// Expectation value <psi|P|psi> of a Pauli-string observable P over `state`.
//
// `pauli` has one character per qubit, each 'I', 'X', 'Y', or 'Z' (lower case
// accepted). Following Qiskit, the string is read right-to-left: the rightmost
// character acts on qubit 0 (the least-significant bit, matching the |...>
// basis-state labels). Its length must equal the number of qubits.
//
// The result is real (Pauli observables are Hermitian) and lies in [-1, 1].
double expval(const State& state, const std::string& pauli);

}  // namespace ket
