// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
//
// GHZ state: the three-qubit maximally entangled state,
// (|000⟩ + |111⟩) / √2. A Hadamard on q0 creates a superposition, then a chain
// of CNOTs spreads it across q1 and q2 — measuring any one qubit instantly
// determines all three.
#include <array>
#include <cstddef>
#include <iostream>

#include <ket/ket.hpp>

int main() {
  ket::Circuit circuit{3};
  circuit.h(0);
  circuit.cnot(0, 1);
  circuit.cnot(1, 2);

  std::cout << "Circuit:\n" << circuit.print() << '\n';

  ket::StateVector state = ket::run(circuit);
  std::cout << "State vector:\n" << state.print() << '\n';

  // Sample measurement outcomes.
  constexpr int shots = 1000;
  std::array<int, 8> counts{};
  for (int i = 0; i < shots; ++i) {
    ++counts[ket::measure(state)];
  }

  std::cout << "Measurements (" << shots << " shots):\n";
  for (std::size_t i = 0; i < counts.size(); ++i) {
    // Bit 2 is q2, bit 1 is q1, bit 0 is q0 — matched to the state vector.
    std::cout << "  |" << ((i >> 2) & 1u) << ((i >> 1) & 1u) << (i & 1u)
              << "⟩: " << counts[i] << '\n';
  }

  return 0;
}
