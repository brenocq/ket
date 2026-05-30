// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
#pragma once

#include <string>
#include <utility>
#include <vector>

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

// A weighted sum of Pauli-string terms — i.e. a Hamiltonian. Each entry is
// (coefficient, pauli_string), e.g. {{0.5, "ZZ"}, {0.3, "XI"}}.
using PauliSum = std::vector<std::pair<double, std::string>>;

// Expectation value <psi|H|psi> of a Hamiltonian H, the coefficient-weighted
// sum of its per-term expectation values.
double expval(const State& state, const PauliSum& hamiltonian);

}  // namespace ket
