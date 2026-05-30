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
#include <vector>

#include <ket/ket.hpp>

int main() {
  ket::Circuit circuit{3};
  circuit.h(0);
  circuit.cx(0, 1);
  circuit.cx(1, 2);
  circuit.measure_all();  // measure qubit i into classical bit i

  std::cout << "Circuit:\n" << circuit.print() << '\n';

  // run() ignores the measurements, so we can still inspect the amplitudes.
  std::cout << "State vector:\n" << ket::run(circuit).print() << '\n';

  // Sample shots from the circuit; each shot fills the classical register.
  constexpr int shots = 1000;
  std::array<int, 8> counts{};
  for (int i = 0; i < shots; ++i) {
    const std::vector<int> creg = ket::sample(circuit);
    const std::size_t outcome =
        static_cast<std::size_t>(creg[0] | (creg[1] << 1) | (creg[2] << 2));
    counts[outcome]++;
  }

  std::cout << "Measurements (" << shots << " shots):\n";
  for (std::size_t i = 0; i < counts.size(); ++i) {
    // Bit 2 is q2, bit 1 is q1, bit 0 is q0 — matched to the state vector.
    std::cout << "  |" << ((i >> 2) & 1u) << ((i >> 1) & 1u) << (i & 1u)
              << "⟩: " << counts[i] << '\n';
  }

  return 0;
}
