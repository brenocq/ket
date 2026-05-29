// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
#include <ket/measurement.hpp>

namespace ket {

std::size_t measure(const StateVector& state, std::mt19937& rng) {
  std::uniform_real_distribution<double> dist(0.0, 1.0);
  const double u = dist(rng);
  double cumulative = 0.0;
  for (std::size_t i = 0; i < state.size(); ++i) {
    cumulative += std::norm(state[i]);
    if (u < cumulative) return i;
  }
  return state.size() - 1;
}

std::size_t measure(const StateVector& state) {
  thread_local std::mt19937 rng{std::random_device{}()};
  return measure(state, rng);
}

}  // namespace ket
