// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
#include <ket/measurement.hpp>

#include <cstddef>

#include <ket/circuit.hpp>
#include <ket/dag.hpp>
#include <ket/gate.hpp>

namespace ket {

std::size_t measure(const State& state, std::mt19937& rng) {
  std::uniform_real_distribution<double> dist(0.0, 1.0);
  const double u = dist(rng);
  double cumulative = 0.0;
  for (std::size_t i = 0; i < state.size(); ++i) {
    cumulative += std::norm(state[i]);
    if (u < cumulative) return i;
  }
  return state.size() - 1;
}

std::size_t measure(const State& state) {
  thread_local std::mt19937 rng{std::random_device{}()};
  return measure(state, rng);
}

std::vector<int> sample(const Circuit& circuit, std::mt19937& rng) {
  const std::size_t outcome = measure(run(circuit), rng);
  std::vector<int> creg(circuit.n_clbits(), 0);
  for (const DagNode& node : circuit.dag().nodes()) {
    const Gate& g = node.gate;
    if (g.type == GateType::Measure) {
      creg[g.clbit] = static_cast<int>((outcome >> g.qubits[0].index) & 1u);
    }
  }
  return creg;
}

std::vector<int> sample(const Circuit& circuit) {
  thread_local std::mt19937 rng{std::random_device{}()};
  return sample(circuit, rng);
}

}  // namespace ket
