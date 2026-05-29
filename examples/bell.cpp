// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
//
// Bell state: the canonical two-qubit maximally entangled state,
// (|00⟩ + |11⟩) / √2. A Hadamard on q0 creates a superposition, and a CNOT
// then entangles q1 with q0 — measuring one qubit instantly determines the
// other.
#include <array>
#include <cstddef>
#include <iostream>
#include <random>

#include <ket/ket.hpp>

int main() {
  ket::Circuit circuit{2};
  circuit.h(0);
  circuit.cnot(0, 1);

  std::cout << "Circuit:\n" << circuit.print() << '\n';

  ket::StateVector state = ket::run(circuit);
  std::cout << "State vector:\n" << state.print() << '\n';

  // Sample measurement outcomes.
  constexpr int shots = 1000;
  std::array<int, 4> counts{};
  for (int i = 0; i < shots; ++i) {
    ++counts[ket::measure(state)];
  }

  std::cout << "Measurements (" << shots << " shots):\n";
  for (std::size_t i = 0; i < counts.size(); ++i) {
    // Bit 1 is q1, bit 0 is q0 — matched to the state vector's labelling.
    std::cout << "  |" << ((i >> 1) & 1u) << (i & 1u) << "⟩: " << counts[i]
              << '\n';
  }

  return 0;
}
