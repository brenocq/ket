// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
#include <ket/simulator.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include <ket/circuit.hpp>
#include <ket/dag.hpp>
#include <ket/gate.hpp>
#include <ket/qubit.hpp>

#include "parallel.hpp"

namespace ket {
namespace {

void apply_single(State& s, std::size_t qubit, Complex m00, Complex m01,
                  Complex m10, Complex m11) {
  const std::size_t mask = std::size_t{1} << qubit;
  internal::parallel_for(s.size(), [&](std::size_t lo, std::size_t hi) {
    for (std::size_t k = lo; k < hi; ++k) {
      if ((k & mask) != 0) continue;
      const std::size_t k1 = k | mask;
      const Complex a = s[k];
      const Complex b = s[k1];
      s[k] = m00 * a + m01 * b;
      s[k1] = m10 * a + m11 * b;
    }
  });
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
  internal::parallel_for(s.size(), [&](std::size_t lo, std::size_t hi) {
    for (std::size_t k = lo; k < hi; ++k) {
      if ((k & cmask) == 0) continue;
      if ((k & tmask) != 0) continue;
      std::swap(s[k], s[k | tmask]);
    }
  });
}

// Controlled-Y: apply Y to the target when the control is 1.
void apply_cy(State& s, std::size_t control, std::size_t target) {
  const std::size_t cmask = std::size_t{1} << control;
  const std::size_t tmask = std::size_t{1} << target;
  internal::parallel_for(s.size(), [&](std::size_t lo, std::size_t hi) {
    for (std::size_t k = lo; k < hi; ++k) {
      if ((k & cmask) == 0 || (k & tmask) != 0)
        continue;  // control=1, target=0
      const std::size_t k1 = k | tmask;
      const Complex a = s[k];         // target-0 amplitude
      const Complex b = s[k1];        // target-1 amplitude
      s[k] = Complex{0.0, -1.0} * b;  // Y: |0> <- -i * (target-1)
      s[k1] = Complex{0.0, 1.0} * a;  // Y: |1> <- +i * (target-0)
    }
  });
}

// Controlled-H: apply H to the target when the control is 1.
void apply_ch(State& s, std::size_t control, std::size_t target) {
  const std::size_t cmask = std::size_t{1} << control;
  const std::size_t tmask = std::size_t{1} << target;
  const double inv_sqrt2 = 1.0 / std::sqrt(2.0);
  internal::parallel_for(s.size(), [&](std::size_t lo, std::size_t hi) {
    for (std::size_t k = lo; k < hi; ++k) {
      if ((k & cmask) == 0 || (k & tmask) != 0)
        continue;  // control=1, target=0
      const std::size_t k1 = k | tmask;
      const Complex a = s[k];
      const Complex b = s[k1];
      s[k] = (a + b) * inv_sqrt2;
      s[k1] = (a - b) * inv_sqrt2;
    }
  });
}

// Apply a 2x2 matrix to the target qubit only on basis states where the
// control is 1 (the controlled analogue of apply_single).
void apply_controlled_single(State& s, std::size_t control, std::size_t target,
                             Complex m00, Complex m01, Complex m10,
                             Complex m11) {
  const std::size_t cmask = std::size_t{1} << control;
  const std::size_t tmask = std::size_t{1} << target;
  internal::parallel_for(s.size(), [&](std::size_t lo, std::size_t hi) {
    for (std::size_t k = lo; k < hi; ++k) {
      if ((k & cmask) == 0 || (k & tmask) != 0)
        continue;  // control=1, target=0
      const std::size_t k1 = k | tmask;
      const Complex a = s[k];
      const Complex b = s[k1];
      s[k] = m00 * a + m01 * b;
      s[k1] = m10 * a + m11 * b;
    }
  });
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
  internal::parallel_for(s.size(), [&](std::size_t lo, std::size_t hi) {
    for (std::size_t k = lo; k < hi; ++k) {
      if ((k & ma) != 0 && (k & mb) == 0) std::swap(s[k], s[(k ^ ma) | mb]);
    }
  });
}

// Toffoli (CCX): flip the target on basis states where both controls are 1.
void apply_ccx(State& s, std::size_t control1, std::size_t control2,
               std::size_t target) {
  const std::size_t c1mask = std::size_t{1} << control1;
  const std::size_t c2mask = std::size_t{1} << control2;
  const std::size_t tmask = std::size_t{1} << target;
  internal::parallel_for(s.size(), [&](std::size_t lo, std::size_t hi) {
    for (std::size_t k = lo; k < hi; ++k) {
      if ((k & c1mask) == 0 || (k & c2mask) == 0)
        continue;                      // both controls = 1
      if ((k & tmask) != 0) continue;  // target = 0
      std::swap(s[k], s[k | tmask]);
    }
  });
}

// Fredkin (CSwap): exchange the two targets on basis states where the control
// is 1 (a controlled apply_swap).
void apply_cswap(State& s, std::size_t control, std::size_t a, std::size_t b) {
  const std::size_t cmask = std::size_t{1} << control;
  const std::size_t ma = std::size_t{1} << a;
  const std::size_t mb = std::size_t{1} << b;
  internal::parallel_for(s.size(), [&](std::size_t lo, std::size_t hi) {
    for (std::size_t k = lo; k < hi; ++k) {
      if ((k & cmask) == 0) continue;  // control = 1
      if ((k & ma) != 0 && (k & mb) == 0) std::swap(s[k], s[(k ^ ma) | mb]);
    }
  });
}

// Controlled phase: multiply by `phase` exactly when both qubits are 1.
// Symmetric in qa/qb (CZ uses phase = -1, CP uses e^{i lambda}).
void apply_cphase(State& s, std::size_t qa, std::size_t qb, Complex phase) {
  const std::size_t ma = std::size_t{1} << qa;
  const std::size_t mb = std::size_t{1} << qb;
  internal::parallel_for(s.size(), [&](std::size_t lo, std::size_t hi) {
    for (std::size_t k = lo; k < hi; ++k) {
      if ((k & ma) != 0 && (k & mb) != 0) s[k] *= phase;
    }
  });
}

// Apply one unitary gate to `s`, acting on the resolved state-vector qubits
// `on` (in g.qubits order). Structural ops (measure/barrier/probe/composite)
// are handled by the driver, not here.
void apply_gate_on(State& s, const Gate& g,
                   const std::vector<std::size_t>& on) {
  switch (g.type) {
    case GateType::H:
      apply_h(s, on[0]);
      break;
    case GateType::X:
      apply_x(s, on[0]);
      break;
    case GateType::Y:
      apply_y(s, on[0]);
      break;
    case GateType::Z:
      apply_z(s, on[0]);
      break;
    case GateType::S:
      apply_s(s, on[0]);
      break;
    case GateType::Sdg:
      apply_sdg(s, on[0]);
      break;
    case GateType::T:
      apply_t(s, on[0]);
      break;
    case GateType::Tdg:
      apply_tdg(s, on[0]);
      break;
    case GateType::Rx:
      apply_rx(s, on[0], g.params[0]);
      break;
    case GateType::Ry:
      apply_ry(s, on[0], g.params[0]);
      break;
    case GateType::Rz:
      apply_rz(s, on[0], g.params[0]);
      break;
    case GateType::U:
      apply_u(s, on[0], g.params[0], g.params[1], g.params[2]);
      break;
    case GateType::CH:
      apply_ch(s, on[0], on[1]);
      break;
    case GateType::CX:
      apply_cx(s, on[0], on[1]);
      break;
    case GateType::CY:
      apply_cy(s, on[0], on[1]);
      break;
    case GateType::CZ:
      apply_cphase(s, on[0], on[1], Complex{-1.0, 0.0});
      break;
    case GateType::CRx:
      apply_crx(s, on[0], on[1], g.params[0]);
      break;
    case GateType::CRy:
      apply_cry(s, on[0], on[1], g.params[0]);
      break;
    case GateType::CRz:
      apply_crz(s, on[0], on[1], g.params[0]);
      break;
    case GateType::CU:
      apply_cu(s, on[0], on[1], g.params[0], g.params[1], g.params[2]);
      break;
    case GateType::CP:
      apply_cphase(s, on[0], on[1],
                   Complex{std::cos(g.params[0]), std::sin(g.params[0])});
      break;
    case GateType::Swap:
      apply_swap(s, on[0], on[1]);
      break;
    case GateType::CCX:
      apply_ccx(s, on[0], on[1], on[2]);
      break;
    case GateType::CSwap:
      apply_cswap(s, on[0], on[1], on[2]);
      break;
    default:
      break;  // non-unitary; handled by the driver
  }
}

bool is_unitary(GateType t) {
  switch (t) {
    case GateType::Measure:
    case GateType::Barrier:
    case GateType::Probe:
    case GateType::Composite:
      return false;
    default:
      return true;
  }
}

// Apply a general m-qubit gate (a 2^m x 2^m matrix, row-major) to `qubits` of
// the state in one pass: for each 2^m-dimensional subspace (the indices that
// differ only in `qubits`), gather the amplitudes, multiply by the matrix, and
// scatter them back. This is the single sweep that gate fusion collapses many
// gates into; bit b of a matrix index corresponds to qubits[b].
void apply_matrix(State& s, const std::vector<std::size_t>& qubits,
                  const std::vector<Complex>& mat) {
  const std::size_t m = qubits.size();
  const std::size_t dim = std::size_t{1} << m;
  std::vector<std::size_t> sorted(qubits);
  std::sort(sorted.begin(), sorted.end());

  // The offset of each in-block index from its group's base is the same for
  // every group, so precompute it once instead of per amplitude.
  std::vector<std::size_t> offset(dim, 0);
  for (std::size_t i = 0; i < dim; ++i) {
    for (std::size_t b = 0; b < m; ++b) {
      if ((i & (std::size_t{1} << b)) != 0)
        offset[i] |= std::size_t{1} << qubits[b];
    }
  }

  internal::parallel_for(s.size() >> m, [&](std::size_t lo, std::size_t hi) {
    std::vector<Complex> in(dim), out(dim);
    for (std::size_t o = lo; o < hi; ++o) {
      // Spread the outer index into a base with a 0 bit at every qubit
      // position.
      std::size_t base = o;
      for (std::size_t q : sorted) {
        base = ((base >> q) << (q + 1)) | (base & ((std::size_t{1} << q) - 1));
      }
      for (std::size_t i = 0; i < dim; ++i) in[i] = s[base | offset[i]];
      for (std::size_t r = 0; r < dim; ++r) {
        Complex acc{0.0, 0.0};
        const Complex* row = &mat[r * dim];
        for (std::size_t c = 0; c < dim; ++c) acc += row[c] * in[c];
        out[r] = acc;
      }
      for (std::size_t i = 0; i < dim; ++i) s[base | offset[i]] = out[i];
    }
  });
}

// Gate fusion: merge a run of consecutive gates acting on the same qubit(s)
// into a single combined gate, so a deep circuit makes fewer sweeps over the
// 2^n state vector. Keeping the block to one gate's worth of qubits preserves
// the streaming memory pattern of the specialized kernels (growing it to span
// distant qubits would make the gather thrash the cache and lose).
class Fuser {
 public:
  explicit Fuser(State& state) : state_(state) {}

  // Add a unitary gate acting on resolved state qubits `on` (g.qubits order).
  // A gate joins the current block only if it acts within the block's qubit set
  // (so the block never spans more qubits than a single gate); a gate touching
  // a new qubit flushes the block and starts a fresh one.
  void add(const Gate& g, std::vector<std::size_t> on) {
    if (!gates_.empty() && !is_subset(on, qubits_)) flush();
    if (gates_.empty())
      qubits_ = on;  // the first gate fixes the block's qubits
    gates_.push_back({&g, std::move(on)});
  }

  void flush() {
    if (gates_.empty()) return;
    if (gates_.size() == 1) {
      apply_gate_on(state_, *gates_[0].gate, gates_[0].on);
    } else if (qubits_.size() == 1) {
      // Collapse a run of single-qubit gates into one 2x2 and apply it with the
      // streaming single-qubit kernel — same memory pattern, one sweep instead
      // of several.
      const std::vector<Complex> m = block_matrix();
      apply_single(state_, qubits_[0], m[0], m[1], m[2], m[3]);
    } else {
      apply_matrix(state_, qubits_, block_matrix());
    }
    gates_.clear();
    qubits_.clear();
  }

 private:
  struct Pending {
    const Gate* gate;
    std::vector<std::size_t> on;
  };

  static bool is_subset(const std::vector<std::size_t>& a,
                        const std::vector<std::size_t>& b) {
    for (std::size_t q : a) {
      if (std::find(b.begin(), b.end(), q) == b.end()) return false;
    }
    return true;
  }

  // The block's combined matrix, column by column: column c is the block
  // applied to basis state |c> in the block's local qubit ordering (qubits_).
  std::vector<Complex> block_matrix() const {
    const std::size_t m = qubits_.size();
    const std::size_t dim = std::size_t{1} << m;
    std::vector<Complex> mat(dim * dim, Complex{0.0, 0.0});
    for (std::size_t col = 0; col < dim; ++col) {
      State e(dim, Complex{0.0, 0.0});
      e[col] = Complex{1.0, 0.0};
      for (const Pending& p : gates_) {
        std::vector<std::size_t> local(p.on.size());
        for (std::size_t i = 0; i < p.on.size(); ++i) {
          local[i] = local_index(p.on[i]);
        }
        apply_gate_on(e, *p.gate, local);
      }
      for (std::size_t row = 0; row < dim; ++row) mat[row * dim + col] = e[row];
    }
    return mat;
  }

  std::size_t local_index(std::size_t state_qubit) const {
    for (std::size_t i = 0; i < qubits_.size(); ++i) {
      if (qubits_[i] == state_qubit) return i;
    }
    return 0;  // unreachable: state_qubit is always in qubits_
  }

  State& state_;
  std::vector<std::size_t> qubits_;  // the block's qubits (state space)
  std::vector<Pending> gates_;
};

// Apply a circuit's gates to the state, fusing runs of unitary gates. `wire[i]`
// is the state-vector qubit that the circuit's qubit i maps to (identity at the
// top level, composed for nested composite blocks).
void apply_circuit(State& state, const Circuit& circuit,
                   const std::vector<std::size_t>& wire,
                   std::vector<std::pair<std::string, State>>* probes) {
  Fuser fuser(state);
  for (const DagNode& node : circuit.dag().nodes()) {
    const Gate& g = node.gate;
    if (is_unitary(g.type)) {
      std::vector<std::size_t> on(g.qubits.size());
      for (std::size_t i = 0; i < g.qubits.size(); ++i) {
        on[i] = wire[g.qubits[i].index];
      }
      fuser.add(g, std::move(on));
      continue;
    }
    fuser.flush();  // structural ops are fusion barriers
    switch (g.type) {
      case GateType::Barrier:
      case GateType::Measure:
        break;
      case GateType::Probe:
        if (probes) probes->emplace_back(g.label, state);  // capture a copy
        break;
      case GateType::Composite: {
        std::vector<std::size_t> sub_wire(g.definition->n_qubits());
        for (std::size_t j = 0; j < g.qubits.size(); ++j) {
          sub_wire[j] = wire[g.qubits[j].index];
        }
        apply_circuit(state, *g.definition, sub_wire, probes);
        break;
      }
      default:
        break;
    }
  }
  fuser.flush();
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
