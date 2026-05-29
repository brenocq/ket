// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
#include <ket/simulator.hpp>

#include <cassert>
#include <cmath>
#include <cstddef>
#include <utility>

#include <ket/circuit.hpp>
#include <ket/dag.hpp>
#include <ket/gate.hpp>
#include <ket/qubit.hpp>

namespace ket {
namespace {

void apply_single(StateVector& s, std::size_t qubit, Complex m00, Complex m01,
                  Complex m10, Complex m11) {
  const std::size_t mask = std::size_t{1} << qubit;
  const std::size_t n = s.size();
  for (std::size_t k = 0; k < n; ++k) {
    if ((k & mask) != 0) continue;
    const std::size_t k1 = k | mask;
    const Complex a = s[k];
    const Complex b = s[k1];
    s[k] = m00 * a + m01 * b;
    s[k1] = m10 * a + m11 * b;
  }
}

void apply_h(StateVector& s, std::size_t q) {
  const double inv_sqrt2 = 1.0 / std::sqrt(2.0);
  apply_single(s, q, inv_sqrt2, inv_sqrt2, inv_sqrt2, -inv_sqrt2);
}

void apply_x(StateVector& s, std::size_t q) {
  apply_single(s, q, 0.0, 1.0, 1.0, 0.0);
}

void apply_z(StateVector& s, std::size_t q) {
  apply_single(s, q, 1.0, 0.0, 0.0, -1.0);
}

void apply_cnot(StateVector& s, std::size_t control, std::size_t target) {
  const std::size_t cmask = std::size_t{1} << control;
  const std::size_t tmask = std::size_t{1} << target;
  const std::size_t n = s.size();
  for (std::size_t k = 0; k < n; ++k) {
    if ((k & cmask) == 0) continue;
    if ((k & tmask) != 0) continue;
    std::swap(s[k], s[k | tmask]);
  }
}

}  // namespace

StateVector run(const Circuit& circuit) {
  const std::size_t dim = std::size_t{1} << circuit.n_qubits();
  StateVector state(dim, Complex{0.0, 0.0});
  state[0] = Complex{1.0, 0.0};

  for (const DagNode& node : circuit.dag().nodes()) {
    const Gate& g = node.gate;
    switch (g.type) {
      case GateType::H:
        assert(g.qubits.size() == 1);
        apply_h(state, g.qubits[0].index);
        break;
      case GateType::X:
        assert(g.qubits.size() == 1);
        apply_x(state, g.qubits[0].index);
        break;
      case GateType::Z:
        assert(g.qubits.size() == 1);
        apply_z(state, g.qubits[0].index);
        break;
      case GateType::CNOT:
        assert(g.qubits.size() == 2);
        apply_cnot(state, g.qubits[0].index, g.qubits[1].index);
        break;
    }
  }

  return state;
}

}  // namespace ket
