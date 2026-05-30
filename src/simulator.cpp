// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
#include <ket/simulator.hpp>

#include <cassert>
#include <cmath>
#include <cstddef>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <ket/circuit.hpp>
#include <ket/dag.hpp>
#include <ket/gate.hpp>
#include <ket/qubit.hpp>

namespace ket {
namespace {

void apply_single(State& s, std::size_t qubit, Complex m00, Complex m01,
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

void apply_h(State& s, std::size_t q) {
  const double inv_sqrt2 = 1.0 / std::sqrt(2.0);
  apply_single(s, q, inv_sqrt2, inv_sqrt2, inv_sqrt2, -inv_sqrt2);
}

void apply_x(State& s, std::size_t q) {
  apply_single(s, q, 0.0, 1.0, 1.0, 0.0);
}

void apply_y(State& s, std::size_t q) {
  apply_single(s, q, 0.0, Complex{0.0, -1.0}, Complex{0.0, 1.0}, 0.0);
}

void apply_z(State& s, std::size_t q) {
  apply_single(s, q, 1.0, 0.0, 0.0, -1.0);
}

void apply_s(State& s, std::size_t q) {
  apply_single(s, q, 1.0, 0.0, 0.0, Complex{0.0, 1.0});  // diag(1, i)
}

void apply_sdg(State& s, std::size_t q) {
  apply_single(s, q, 1.0, 0.0, 0.0, Complex{0.0, -1.0});  // diag(1, -i)
}

void apply_t(State& s, std::size_t q) {
  const double r = 1.0 / std::sqrt(2.0);
  apply_single(s, q, 1.0, 0.0, 0.0, Complex{r, r});  // diag(1, e^{i pi/4})
}

void apply_tdg(State& s, std::size_t q) {
  const double r = 1.0 / std::sqrt(2.0);
  apply_single(s, q, 1.0, 0.0, 0.0, Complex{r, -r});  // diag(1, e^{-i pi/4})
}

void apply_rx(State& s, std::size_t q, double theta) {
  const double c = std::cos(theta / 2.0);
  const double sn = std::sin(theta / 2.0);
  apply_single(s, q, c, Complex{0.0, -sn}, Complex{0.0, -sn}, c);
}

void apply_ry(State& s, std::size_t q, double theta) {
  const double c = std::cos(theta / 2.0);
  const double sn = std::sin(theta / 2.0);
  apply_single(s, q, c, -sn, sn, c);
}

void apply_rz(State& s, std::size_t q, double theta) {
  const double c = std::cos(theta / 2.0);
  const double sn = std::sin(theta / 2.0);
  apply_single(s, q, Complex{c, -sn}, 0.0, 0.0, Complex{c, sn});
}

void apply_cnot(State& s, std::size_t control, std::size_t target) {
  const std::size_t cmask = std::size_t{1} << control;
  const std::size_t tmask = std::size_t{1} << target;
  const std::size_t n = s.size();
  for (std::size_t k = 0; k < n; ++k) {
    if ((k & cmask) == 0) continue;
    if ((k & tmask) != 0) continue;
    std::swap(s[k], s[k | tmask]);
  }
}

// Controlled phase: multiply by `phase` exactly when both qubits are 1.
// Symmetric in qa/qb (CZ uses phase = -1, CP uses e^{i lambda}).
void apply_cphase(State& s, std::size_t qa, std::size_t qb, Complex phase) {
  const std::size_t ma = std::size_t{1} << qa;
  const std::size_t mb = std::size_t{1} << qb;
  const std::size_t n = s.size();
  for (std::size_t k = 0; k < n; ++k) {
    if ((k & ma) != 0 && (k & mb) != 0) s[k] *= phase;
  }
}

// Apply a circuit's gates to the state. `wire[i]` is the actual state-vector
// qubit that the circuit's qubit i maps to (identity at the top level, composed
// for nested composite blocks).
void apply_circuit(State& state, const Circuit& circuit,
                   const std::vector<std::size_t>& wire) {
  for (const DagNode& node : circuit.dag().nodes()) {
    const Gate& g = node.gate;
    switch (g.type) {
      case GateType::H:
        assert(g.qubits.size() == 1);
        apply_h(state, wire[g.qubits[0].index]);
        break;
      case GateType::X:
        assert(g.qubits.size() == 1);
        apply_x(state, wire[g.qubits[0].index]);
        break;
      case GateType::Y:
        assert(g.qubits.size() == 1);
        apply_y(state, wire[g.qubits[0].index]);
        break;
      case GateType::Z:
        assert(g.qubits.size() == 1);
        apply_z(state, wire[g.qubits[0].index]);
        break;
      case GateType::S:
        assert(g.qubits.size() == 1);
        apply_s(state, wire[g.qubits[0].index]);
        break;
      case GateType::Sdg:
        assert(g.qubits.size() == 1);
        apply_sdg(state, wire[g.qubits[0].index]);
        break;
      case GateType::T:
        assert(g.qubits.size() == 1);
        apply_t(state, wire[g.qubits[0].index]);
        break;
      case GateType::Tdg:
        assert(g.qubits.size() == 1);
        apply_tdg(state, wire[g.qubits[0].index]);
        break;
      case GateType::CNOT:
        assert(g.qubits.size() == 2);
        apply_cnot(state, wire[g.qubits[0].index], wire[g.qubits[1].index]);
        break;
      case GateType::CZ:
        assert(g.qubits.size() == 2);
        apply_cphase(state, wire[g.qubits[0].index], wire[g.qubits[1].index],
                     Complex{-1.0, 0.0});
        break;
      case GateType::CP:
        assert(g.qubits.size() == 2 && !g.params.empty());
        apply_cphase(state, wire[g.qubits[0].index], wire[g.qubits[1].index],
                     Complex{std::cos(g.params[0]), std::sin(g.params[0])});
        break;
      case GateType::Rx:
        assert(g.qubits.size() == 1 && !g.params.empty());
        apply_rx(state, wire[g.qubits[0].index], g.params[0]);
        break;
      case GateType::Ry:
        assert(g.qubits.size() == 1 && !g.params.empty());
        apply_ry(state, wire[g.qubits[0].index], g.params[0]);
        break;
      case GateType::Rz:
        assert(g.qubits.size() == 1 && !g.params.empty());
        apply_rz(state, wire[g.qubits[0].index], g.params[0]);
        break;
      case GateType::Barrier:
        break;  // no-op: barriers only affect ordering and rendering
      case GateType::Measure:
        break;  // no-op here: run() returns the pre-measurement state vector
      case GateType::Composite: {
        assert(g.definition);
        std::vector<std::size_t> sub_wire(g.definition->n_qubits());
        for (std::size_t j = 0; j < g.qubits.size(); ++j) {
          sub_wire[j] = wire[g.qubits[j].index];
        }
        apply_circuit(state, *g.definition, sub_wire);
        break;
      }
    }
  }
}

}  // namespace

State run(const Circuit& circuit) {
  const std::size_t dim = std::size_t{1} << circuit.n_qubits();
  State state(dim, Complex{0.0, 0.0});
  state[0] = Complex{1.0, 0.0};

  std::vector<std::size_t> wire(circuit.n_qubits());
  for (std::size_t i = 0; i < wire.size(); ++i) wire[i] = i;
  apply_circuit(state, circuit, wire);

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

std::string State::print() const {
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
