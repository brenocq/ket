// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
#include <ket/simulator.hpp>

#include <cassert>
#include <cmath>
#include <cstddef>
#include <sstream>
#include <string>
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

namespace {

std::string ket_label(std::size_t index, std::size_t n_qubits) {
  std::string bits(n_qubits, '0');
  for (std::size_t b = 0; b < n_qubits; ++b) {
    if ((index >> b) & 1u) bits[n_qubits - 1 - b] = '1';
  }
  return "|" + bits + "⟩";  // |bits⟩
}

std::string format_amplitude(Complex z) {
  std::ostringstream os;
  if (z.imag() == 0.0) {
    os << z.real();
  } else if (z.real() == 0.0) {
    os << z.imag() << "i";
  } else if (z.imag() < 0.0) {
    os << z.real() << " - " << -z.imag() << "i";
  } else {
    os << z.real() << " + " << z.imag() << "i";
  }
  return os.str();
}

std::size_t qubit_count(std::size_t dim) {
  std::size_t n = 0;
  while ((std::size_t{1} << n) < dim) ++n;
  return n;
}

}  // namespace

std::string StateVector::print() const {
  const std::size_t n = qubit_count(size());
  std::string result;
  for (std::size_t i = 0; i < size(); ++i) {
    result += ket_label(i, n);
    result += ": ";
    result += format_amplitude(amplitudes_[i]);
    result += '\n';
  }
  return result;
}

}  // namespace ket
