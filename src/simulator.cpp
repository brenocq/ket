// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
#include <ket/simulator.hpp>

#include <cassert>
#include <cmath>
#include <cstddef>
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

// General single-qubit gate U(theta, phi, lambda) (the OpenQASM primitive).
void apply_u(State& s, std::size_t q, double theta, double phi, double lambda) {
  const double c = std::cos(theta / 2.0);
  const double sn = std::sin(theta / 2.0);
  apply_single(s, q, Complex{c, 0.0}, -std::polar(sn, lambda),
               std::polar(sn, phi), std::polar(c, phi + lambda));
}

void apply_cx(State& s, std::size_t control, std::size_t target) {
  const std::size_t cmask = std::size_t{1} << control;
  const std::size_t tmask = std::size_t{1} << target;
  const std::size_t n = s.size();
  for (std::size_t k = 0; k < n; ++k) {
    if ((k & cmask) == 0) continue;
    if ((k & tmask) != 0) continue;
    std::swap(s[k], s[k | tmask]);
  }
}

// Controlled-Y: apply Y to the target when the control is 1.
void apply_cy(State& s, std::size_t control, std::size_t target) {
  const std::size_t cmask = std::size_t{1} << control;
  const std::size_t tmask = std::size_t{1} << target;
  const std::size_t n = s.size();
  for (std::size_t k = 0; k < n; ++k) {
    if ((k & cmask) == 0 || (k & tmask) != 0) continue;  // control=1, target=0
    const std::size_t k1 = k | tmask;
    const Complex a = s[k];         // target-0 amplitude
    const Complex b = s[k1];        // target-1 amplitude
    s[k] = Complex{0.0, -1.0} * b;  // Y: |0> <- -i * (target-1)
    s[k1] = Complex{0.0, 1.0} * a;  // Y: |1> <- +i * (target-0)
  }
}

// Controlled-H: apply H to the target when the control is 1.
void apply_ch(State& s, std::size_t control, std::size_t target) {
  const std::size_t cmask = std::size_t{1} << control;
  const std::size_t tmask = std::size_t{1} << target;
  const double inv_sqrt2 = 1.0 / std::sqrt(2.0);
  const std::size_t n = s.size();
  for (std::size_t k = 0; k < n; ++k) {
    if ((k & cmask) == 0 || (k & tmask) != 0) continue;  // control=1, target=0
    const std::size_t k1 = k | tmask;
    const Complex a = s[k];
    const Complex b = s[k1];
    s[k] = (a + b) * inv_sqrt2;
    s[k1] = (a - b) * inv_sqrt2;
  }
}

// Apply a 2x2 matrix to the target qubit only on basis states where the
// control is 1 (the controlled analogue of apply_single).
void apply_controlled_single(State& s, std::size_t control, std::size_t target,
                             Complex m00, Complex m01, Complex m10,
                             Complex m11) {
  const std::size_t cmask = std::size_t{1} << control;
  const std::size_t tmask = std::size_t{1} << target;
  const std::size_t n = s.size();
  for (std::size_t k = 0; k < n; ++k) {
    if ((k & cmask) == 0 || (k & tmask) != 0) continue;  // control=1, target=0
    const std::size_t k1 = k | tmask;
    const Complex a = s[k];
    const Complex b = s[k1];
    s[k] = m00 * a + m01 * b;
    s[k1] = m10 * a + m11 * b;
  }
}

// Controlled rotations: the rotation matrix is applied to the target only when
// the control is 1 (same matrices as apply_rx/apply_ry/apply_rz).
void apply_crx(State& s, std::size_t control, std::size_t target,
               double theta) {
  const double c = std::cos(theta / 2.0);
  const double sn = std::sin(theta / 2.0);
  apply_controlled_single(s, control, target, c, Complex{0.0, -sn},
                          Complex{0.0, -sn}, c);
}

void apply_cry(State& s, std::size_t control, std::size_t target,
               double theta) {
  const double c = std::cos(theta / 2.0);
  const double sn = std::sin(theta / 2.0);
  apply_controlled_single(s, control, target, c, -sn, sn, c);
}

void apply_crz(State& s, std::size_t control, std::size_t target,
               double theta) {
  const double c = std::cos(theta / 2.0);
  const double sn = std::sin(theta / 2.0);
  apply_controlled_single(s, control, target, Complex{c, -sn}, 0.0, 0.0,
                          Complex{c, sn});
}

// Controlled general unitary: apply U(theta, phi, lambda) to the target when
// the control is 1 (same matrix as apply_u).
void apply_cu(State& s, std::size_t control, std::size_t target, double theta,
              double phi, double lambda) {
  const double c = std::cos(theta / 2.0);
  const double sn = std::sin(theta / 2.0);
  apply_controlled_single(s, control, target, Complex{c, 0.0},
                          -std::polar(sn, lambda), std::polar(sn, phi),
                          std::polar(c, phi + lambda));
}

// SWAP: exchange amplitudes of basis states that differ in exactly the two
// qubits (|..1..0..> <-> |..0..1..>).
void apply_swap(State& s, std::size_t qa, std::size_t qb) {
  const std::size_t ma = std::size_t{1} << qa;
  const std::size_t mb = std::size_t{1} << qb;
  const std::size_t n = s.size();
  for (std::size_t k = 0; k < n; ++k) {
    if ((k & ma) != 0 && (k & mb) == 0) std::swap(s[k], s[(k ^ ma) | mb]);
  }
}

// Toffoli (CCX): flip the target on basis states where both controls are 1.
void apply_ccx(State& s, std::size_t control1, std::size_t control2,
               std::size_t target) {
  const std::size_t c1mask = std::size_t{1} << control1;
  const std::size_t c2mask = std::size_t{1} << control2;
  const std::size_t tmask = std::size_t{1} << target;
  const std::size_t n = s.size();
  for (std::size_t k = 0; k < n; ++k) {
    if ((k & c1mask) == 0 || (k & c2mask) == 0) continue;  // both controls = 1
    if ((k & tmask) != 0) continue;                        // target = 0
    std::swap(s[k], s[k | tmask]);
  }
}

// Fredkin (CSwap): exchange the two targets on basis states where the control
// is 1 (a controlled apply_swap).
void apply_cswap(State& s, std::size_t control, std::size_t a, std::size_t b) {
  const std::size_t cmask = std::size_t{1} << control;
  const std::size_t ma = std::size_t{1} << a;
  const std::size_t mb = std::size_t{1} << b;
  const std::size_t n = s.size();
  for (std::size_t k = 0; k < n; ++k) {
    if ((k & cmask) == 0) continue;  // control = 1
    if ((k & ma) != 0 && (k & mb) == 0) std::swap(s[k], s[(k ^ ma) | mb]);
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
                   const std::vector<std::size_t>& wire,
                   std::vector<std::pair<std::string, State>>* probes) {
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
      case GateType::CH:
        assert(g.qubits.size() == 2);
        apply_ch(state, wire[g.qubits[0].index], wire[g.qubits[1].index]);
        break;
      case GateType::CX:
        assert(g.qubits.size() == 2);
        apply_cx(state, wire[g.qubits[0].index], wire[g.qubits[1].index]);
        break;
      case GateType::CY:
        assert(g.qubits.size() == 2);
        apply_cy(state, wire[g.qubits[0].index], wire[g.qubits[1].index]);
        break;
      case GateType::CZ:
        assert(g.qubits.size() == 2);
        apply_cphase(state, wire[g.qubits[0].index], wire[g.qubits[1].index],
                     Complex{-1.0, 0.0});
        break;
      case GateType::CRx:
        assert(g.qubits.size() == 2 && !g.params.empty());
        apply_crx(state, wire[g.qubits[0].index], wire[g.qubits[1].index],
                  g.params[0]);
        break;
      case GateType::CRy:
        assert(g.qubits.size() == 2 && !g.params.empty());
        apply_cry(state, wire[g.qubits[0].index], wire[g.qubits[1].index],
                  g.params[0]);
        break;
      case GateType::CRz:
        assert(g.qubits.size() == 2 && !g.params.empty());
        apply_crz(state, wire[g.qubits[0].index], wire[g.qubits[1].index],
                  g.params[0]);
        break;
      case GateType::CU:
        assert(g.qubits.size() == 2 && g.params.size() == 3);
        apply_cu(state, wire[g.qubits[0].index], wire[g.qubits[1].index],
                 g.params[0], g.params[1], g.params[2]);
        break;
      case GateType::CP:
        assert(g.qubits.size() == 2 && !g.params.empty());
        apply_cphase(state, wire[g.qubits[0].index], wire[g.qubits[1].index],
                     Complex{std::cos(g.params[0]), std::sin(g.params[0])});
        break;
      case GateType::Swap:
        assert(g.qubits.size() == 2);
        apply_swap(state, wire[g.qubits[0].index], wire[g.qubits[1].index]);
        break;
      case GateType::CCX:
        assert(g.qubits.size() == 3);
        apply_ccx(state, wire[g.qubits[0].index], wire[g.qubits[1].index],
                  wire[g.qubits[2].index]);
        break;
      case GateType::CSwap:
        assert(g.qubits.size() == 3);
        apply_cswap(state, wire[g.qubits[0].index], wire[g.qubits[1].index],
                    wire[g.qubits[2].index]);
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
      case GateType::U:
        assert(g.qubits.size() == 1 && g.params.size() == 3);
        apply_u(state, wire[g.qubits[0].index], g.params[0], g.params[1],
                g.params[2]);
        break;
      case GateType::Barrier:
        break;  // no-op: barriers only affect ordering and rendering
      case GateType::Measure:
        break;  // no-op here: run() returns the pre-measurement state vector
      case GateType::Probe:
        if (probes) probes->emplace_back(g.label, state);  // capture a copy
        break;
      case GateType::Composite: {
        assert(g.definition);
        std::vector<std::size_t> sub_wire(g.definition->n_qubits());
        for (std::size_t j = 0; j < g.qubits.size(); ++j) {
          sub_wire[j] = wire[g.qubits[j].index];
        }
        apply_circuit(state, *g.definition, sub_wire, probes);
        break;
      }
    }
  }
}

}  // namespace

namespace {

std::vector<std::size_t> identity_wire(std::size_t n) {
  std::vector<std::size_t> wire(n);
  for (std::size_t i = 0; i < n; ++i) wire[i] = i;
  return wire;
}

State ground_state(std::size_t n_qubits) {
  State state(std::size_t{1} << n_qubits, Complex{0.0, 0.0});
  state[0] = Complex{1.0, 0.0};
  return state;
}

}  // namespace

State run(const Circuit& circuit) {
  State state = ground_state(circuit.n_qubits());
  apply_circuit(state, circuit, identity_wire(circuit.n_qubits()), nullptr);
  return state;
}

ProbeRun run_with_probes(const Circuit& circuit) {
  State state = ground_state(circuit.n_qubits());
  std::vector<std::pair<std::string, State>> probes;
  apply_circuit(state, circuit, identity_wire(circuit.n_qubits()), &probes);
  return ProbeRun{std::move(state), std::move(probes)};
}

}  // namespace ket
