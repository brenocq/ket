// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
#include <ket/measurement.hpp>

#include <cstddef>

#include <ket/backends/backend.hpp>
#include <ket/circuit.hpp>

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

std::vector<int> sample(const Circuit& circuit, std::mt19937& rng,
                        Method method) {
  return simulate(circuit, method)->sample(rng);
}

std::vector<int> sample(const Circuit& circuit, Method method) {
  thread_local std::mt19937 rng{std::random_device{}()};
  return sample(circuit, rng, method);
}

}  // namespace ket
