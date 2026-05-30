// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
//
// Bernstein–Vazirani algorithm. A hidden n-bit string s defines an oracle
// f(x) = s·x (mod 2) — the parity of the input bits where s has a 1. The
// algorithm recovers all n bits of s with a SINGLE oracle query; a classical
// algorithm needs n queries (one bit of s per query).
//
// As in Deutsch–Jozsa, the inputs go into a uniform superposition and the
// ancilla into |−⟩, so the oracle kicks back a phase (−1)^(s·x). A final layer
// of Hadamards decodes that phase pattern directly into the basis state |s⟩,
// so measuring the input register yields s.
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

#include <ket/ket.hpp>

namespace {

constexpr std::size_t kInputs = 3;                          // input qubits
constexpr std::size_t kAncilla = kInputs;                   // ancilla index
constexpr std::size_t kQubits = kInputs + 1;
constexpr std::size_t kInputMask = (std::size_t{1} << kInputs) - 1;

// Build U_f for f(x) = s·x as an f(x) block: one CNOT onto the ancilla from
// each input qubit i where bit i of the secret is set.
ket::Circuit oracle_block(std::size_t secret) {
  ket::Circuit f{kQubits, "f(x)"};
  for (std::size_t i = 0; i < kInputs; ++i) {
    if ((secret >> i) & 1u) f.cnot(i, kAncilla);
  }
  return f;
}

ket::Circuit bernstein_vazirani(std::size_t secret) {
  std::vector<std::size_t> all(kQubits);
  for (std::size_t q = 0; q < kQubits; ++q) all[q] = q;

  ket::Circuit c{kQubits};
  c.x(kAncilla);                                            // ancilla -> |1⟩
  for (std::size_t q = 0; q < kQubits; ++q) c.h(q);         // H on all qubits
  c.append(oracle_block(secret), all);                      // U_f as a block
  for (std::size_t q = 0; q < kInputs; ++q) c.h(q);         // H on inputs
  return c;
}

// Render a value as kInputs bits, most-significant qubit first (matching the
// state vector's labelling: bit i is qubit i).
std::string to_bits(std::size_t value) {
  std::string s(kInputs, '0');
  for (std::size_t i = 0; i < kInputs; ++i) {
    if ((value >> i) & 1u) s[kInputs - 1 - i] = '1';
  }
  return s;
}

}  // namespace

int main() {
  std::cout << "Bernstein-Vazirani with " << kInputs
            << " input qubits + 1 ancilla.\n"
            << "Recovers a hidden s where f(x) = s.x (mod 2), in one query.\n\n";

  // Show the circuit and state for one secret. The oracle is an f(x) block;
  // decompose() reveals the CNOTs inside it.
  const std::size_t demo = 0b101;
  ket::Circuit demo_circuit = bernstein_vazirani(demo);
  std::cout << "Example circuit (s = " << to_bits(demo) << "):\n"
            << demo_circuit.print() << '\n';
  std::cout << "With the f(x) block expanded:\n"
            << demo_circuit.decompose().print() << '\n';
  std::cout << "State before measuring the input register:\n"
            << ket::run(demo_circuit).print() << '\n';

  // The input register collapses deterministically to s, so one measurement
  // recovers the whole string.
  std::cout << "Results (measured input register = s):\n";
  for (std::size_t secret = 0; secret < (std::size_t{1} << kInputs); ++secret) {
    ket::State state = ket::run(bernstein_vazirani(secret));
    const std::size_t recovered = ket::measure(state) & kInputMask;
    std::cout << "  s = " << to_bits(secret) << "  ->  recovered "
              << to_bits(recovered) << "  ["
              << (recovered == secret ? "OK" : "WRONG") << "]\n";
  }

  return 0;
}
