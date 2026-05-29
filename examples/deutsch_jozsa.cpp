// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
//
// Deutsch–Jozsa algorithm. Given a function f: {0,1}^n -> {0,1} promised to be
// either *constant* (same value for every input) or *balanced* (0 on half the
// inputs, 1 on the other half), it decides which with a single oracle query —
// a classical algorithm may need up to 2^(n-1)+1 queries.
//
// The circuit puts the n input qubits in a uniform superposition and the
// ancilla in |1⟩. With the ancilla in |−⟩, the oracle U_f: |x⟩|y⟩ -> |x⟩|y⊕f(x)⟩
// kicks a phase (−1)^f(x) back onto the inputs. A final layer of Hadamards then
// concentrates the inputs onto |0…0⟩ exactly when f is constant.
#include <cstddef>
#include <functional>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include <ket/ket.hpp>

namespace {

constexpr std::size_t kInputs = 3;                          // input qubits
constexpr std::size_t kAncilla = kInputs;                   // ancilla index
constexpr std::size_t kQubits = kInputs + 1;
constexpr std::size_t kInputMask = (std::size_t{1} << kInputs) - 1;

// An oracle applies U_f to a circuit using only H/X/CNOT gates.
using Oracle = std::function<void(ket::Circuit&)>;

ket::Circuit deutsch_jozsa(const Oracle& oracle) {
  ket::Circuit c{kQubits};
  c.x(kAncilla);                                            // ancilla -> |1⟩
  for (std::size_t q = 0; q < kQubits; ++q) c.h(q);         // H on all qubits
  oracle(c);                                                // U_f
  for (std::size_t q = 0; q < kInputs; ++q) c.h(q);         // H on inputs
  return c;
}

}  // namespace

int main() {
  struct Case {
    std::string name;
    bool balanced;
    Oracle oracle;
  };

  const std::vector<Case> cases = {
      {"constant  f(x) = 0", false, [](ket::Circuit&) {}},
      {"constant  f(x) = 1", false, [](ket::Circuit& c) { c.x(kAncilla); }},
      {"balanced  f(x) = x0", true,
       [](ket::Circuit& c) { c.cnot(0, kAncilla); }},
      {"balanced  f(x) = x0^x1^x2", true,
       [](ket::Circuit& c) {
         for (std::size_t q = 0; q < kInputs; ++q) c.cnot(q, kAncilla);
       }},
  };

  std::cout << "Deutsch-Jozsa with " << kInputs
            << " input qubits + 1 ancilla.\n"
            << "Decides constant vs balanced in a single query.\n\n";

  // Show the circuit and final state for one balanced oracle.
  const Case& demo = cases.back();
  ket::Circuit demo_circuit = deutsch_jozsa(demo.oracle);
  std::cout << "Example circuit (" << demo.name << "):\n"
            << demo_circuit.print() << '\n';
  std::cout << "State before measuring the input register:\n"
            << ket::run(demo_circuit).print() << '\n';

  // The input register is all-zero iff f is constant; the ancilla, left in
  // |−⟩, is ignored via the input mask. The outcome is deterministic, so one
  // measurement decides it.
  std::cout << "Results:\n";
  for (const Case& test : cases) {
    ket::StateVector state = ket::run(deutsch_jozsa(test.oracle));
    const bool balanced = (ket::measure(state) & kInputMask) != 0;
    std::cout << "  " << std::left << std::setw(26) << test.name << " -> "
              << (balanced ? "balanced" : "constant") << "  ["
              << (balanced == test.balanced ? "OK" : "WRONG") << "]\n";
  }

  return 0;
}
