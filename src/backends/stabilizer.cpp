// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
//
// The stabilizer backend: simulate Clifford circuits in O(n^2) memory and time
// using the Aaronson-Gottesman tableau ("Improved Simulation of Stabilizer
// Circuits", 2004), instead of a 2^n state vector. It exposes the same
// representation-agnostic queries as the dense backend — measurement sampling
// and Pauli expectation values — never a full state vector.
#include <cstddef>
#include <cstdint>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <ket/backends/backend.hpp>
#include <ket/circuit.hpp>
#include <ket/dag.hpp>
#include <ket/gate.hpp>

#include "internal.hpp"

namespace ket {
namespace {

// A stabilizer tableau over n qubits. Rows 0..n-1 are the destabilizer
// generators, rows n..2n-1 the stabilizer generators, and row 2n a scratch row
// used by measurement and expectation queries. Each row encodes a Pauli
// operator (-1)^r * prod_j X_j^{x_j} Z_j^{z_j}.
class Tableau {
 public:
  explicit Tableau(std::size_t n)
      : n_(n), x_(rows() * n, 0), z_(rows() * n, 0), r_(rows(), 0) {
    // The |0...0> stabilizer state: destabilizer i = X_i, stabilizer i = Z_i.
    for (std::size_t i = 0; i < n_; ++i) {
      x(i, i) = 1;       // destabilizers
      z(n_ + i, i) = 1;  // stabilizers
    }
  }

  std::size_t n() const { return n_; }

  // --- Clifford gates ------------------------------------------------------
  // The three primitives carry the full tableau update; the rest compose them.
  void h(std::size_t q) {
    for (std::size_t i = 0; i < 2 * n_; ++i) {
      r(i) ^= static_cast<std::uint8_t>(x(i, q) & z(i, q));
      std::swap(x(i, q), z(i, q));
    }
  }
  void s(std::size_t q) {
    for (std::size_t i = 0; i < 2 * n_; ++i) {
      r(i) ^= static_cast<std::uint8_t>(x(i, q) & z(i, q));
      z(i, q) ^= x(i, q);
    }
  }
  void cnot(std::size_t a, std::size_t b) {
    for (std::size_t i = 0; i < 2 * n_; ++i) {
      r(i) ^= static_cast<std::uint8_t>(x(i, a) & z(i, b) &
                                        (x(i, b) ^ z(i, a) ^ 1u));
      x(i, b) ^= x(i, a);
      z(i, a) ^= z(i, b);
    }
  }
  // Pauli gates only flip generator signs (X anticommutes with Z/Y, etc.).
  void x_gate(std::size_t q) {
    for (std::size_t i = 0; i < 2 * n_; ++i)
      r(i) = static_cast<std::uint8_t>(r(i) ^ z(i, q));
  }
  void z_gate(std::size_t q) {
    for (std::size_t i = 0; i < 2 * n_; ++i)
      r(i) = static_cast<std::uint8_t>(r(i) ^ x(i, q));
  }
  void y_gate(std::size_t q) {
    for (std::size_t i = 0; i < 2 * n_; ++i)
      r(i) = static_cast<std::uint8_t>(r(i) ^ x(i, q) ^ z(i, q));
  }
  void sdg(std::size_t q) {
    s(q);
    z_gate(q);  // S^dagger = S^3 = S . Z
  }
  void cz(std::size_t a, std::size_t b) {
    h(b);
    cnot(a, b);
    h(b);
  }
  void cy(std::size_t a, std::size_t b) {
    sdg(b);
    cnot(a, b);
    s(b);  // CY = (I (x) S) CX (I (x) S^dagger)
  }
  void swap(std::size_t a, std::size_t b) {
    cnot(a, b);
    cnot(b, a);
    cnot(a, b);
  }

  // Measure qubit q in the computational basis, collapsing the state and
  // returning the outcome bit (0 or 1).
  int measure(std::size_t q, std::mt19937& rng) {
    std::size_t p =
        2 * n_;  // a stabilizer generator that anticommutes with Z_q
    for (std::size_t i = n_; i < 2 * n_; ++i) {
      if (x(i, q)) {
        p = i;
        break;
      }
    }
    if (p != 2 * n_) {  // random outcome
      for (std::size_t i = 0; i < 2 * n_; ++i) {
        if (i != p && x(i, q)) rowsum(i, p);
      }
      copy_row(p - n_, p);  // destabilizer <- old stabilizer
      clear_row(p);
      z(p, q) = 1;
      r(p) = static_cast<std::uint8_t>(rng() & 1u);
      return r(p);
    }
    // deterministic outcome: accumulate the relevant stabilizers into scratch
    clear_row(2 * n_);
    for (std::size_t i = 0; i < n_; ++i) {
      if (x(i, q)) rowsum(2 * n_, i + n_);
    }
    return r(2 * n_);
  }

  // <psi|P|psi> for a Pauli operator given as per-qubit X/Z exponents. Returns
  // 0 if P anticommutes with any stabilizer, otherwise +/-1. P is rotated to a
  // pure Z-string by the caller, so there are no Y phase factors to track here.
  double z_string_expectation(const std::vector<std::size_t>& support) {
    // P = prod_{q in support} Z_q. It anticommutes with a generator iff the
    // generator has an X on an odd number of the support qubits.
    auto anticommutes = [&](std::size_t row) {
      std::uint8_t ac = 0;
      for (std::size_t q : support) ac ^= x(row, q);
      return ac != 0;
    };
    for (std::size_t i = n_; i < 2 * n_; ++i) {
      if (anticommutes(i)) return 0.0;  // not in the stabilizer group
    }
    // P commutes with every stabilizer, so it equals a signed product of them;
    // the destabilizers select which, and rowsum accumulates the sign.
    clear_row(2 * n_);
    for (std::size_t i = 0; i < n_; ++i) {
      if (anticommutes(i)) rowsum(2 * n_, i + n_);
    }
    return r(2 * n_) ? -1.0 : 1.0;
  }

 private:
  std::size_t rows() const { return 2 * n_ + 1; }
  std::uint8_t& x(std::size_t row, std::size_t col) {
    return x_[row * n_ + col];
  }
  std::uint8_t& z(std::size_t row, std::size_t col) {
    return z_[row * n_ + col];
  }
  std::uint8_t& r(std::size_t row) { return r_[row]; }

  void clear_row(std::size_t row) {
    for (std::size_t j = 0; j < n_; ++j) {
      x(row, j) = 0;
      z(row, j) = 0;
    }
    r(row) = 0;
  }
  void copy_row(std::size_t dst, std::size_t src) {
    for (std::size_t j = 0; j < n_; ++j) {
      x(dst, j) = x(src, j);
      z(dst, j) = z(src, j);
    }
    r(dst) = r(src);
  }

  // Power of i (mod 4) produced when left-multiplying Pauli (x1,z1) by (x2,z2).
  static int g(std::uint8_t x1, std::uint8_t z1, std::uint8_t x2,
               std::uint8_t z2) {
    if (!x1 && !z1) return 0;                        // I
    if (x1 && z1) return static_cast<int>(z2) - x2;  // Y
    if (x1 && !z1) return z2 * (2 * x2 - 1);         // X
    return x2 * (1 - 2 * z2);                        // Z
  }

  // Left-multiply row h by row i, storing the result (with its sign) in row h.
  void rowsum(std::size_t h, std::size_t i) {
    int sum = 2 * r(h) + 2 * r(i);
    for (std::size_t j = 0; j < n_; ++j) {
      sum += g(x(i, j), z(i, j), x(h, j), z(h, j));
    }
    sum %= 4;
    if (sum < 0) sum += 4;
    r(h) = static_cast<std::uint8_t>(sum == 2 ? 1
                                              : 0);  // products are real (+/-1)
    for (std::size_t j = 0; j < n_; ++j) {
      x(h, j) ^= x(i, j);
      z(h, j) ^= z(i, j);
    }
  }

  std::size_t n_;
  std::vector<std::uint8_t> x_;
  std::vector<std::uint8_t> z_;
  std::vector<std::uint8_t> r_;
};

// Apply a (Clifford) circuit to the tableau. `wire[i]` maps the circuit's qubit
// i to a tableau qubit, composed for nested composite blocks (mirrors the dense
// simulator's apply_circuit).
void evolve(Tableau& t, const Circuit& circuit,
            const std::vector<std::size_t>& wire) {
  for (const DagNode& node : circuit.dag().nodes()) {
    const Gate& g = node.gate;
    auto q = [&](std::size_t k) { return wire[g.qubits[k].index]; };
    switch (g.type) {
      case GateType::H:
        t.h(q(0));
        break;
      case GateType::X:
        t.x_gate(q(0));
        break;
      case GateType::Y:
        t.y_gate(q(0));
        break;
      case GateType::Z:
        t.z_gate(q(0));
        break;
      case GateType::S:
        t.s(q(0));
        break;
      case GateType::Sdg:
        t.sdg(q(0));
        break;
      case GateType::CX:
        t.cnot(q(0), q(1));
        break;
      case GateType::CY:
        t.cy(q(0), q(1));
        break;
      case GateType::CZ:
        t.cz(q(0), q(1));
        break;
      case GateType::Swap:
        t.swap(q(0), q(1));
        break;
      case GateType::Measure:
      case GateType::Barrier:
      case GateType::Probe:
        break;  // structural: no effect on the (terminal-measured) state
      case GateType::Composite: {
        std::vector<std::size_t> sub_wire(g.definition->n_qubits());
        for (std::size_t j = 0; j < g.qubits.size(); ++j) {
          sub_wire[j] = wire[g.qubits[j].index];
        }
        evolve(t, *g.definition, sub_wire);
        break;
      }
      default:
        // is_clifford() gates this backend, so this is unreachable in practice.
        throw std::invalid_argument(
            "stabilizer backend: non-Clifford gate encountered");
    }
  }
}

class StabilizerSimulation final : public Simulation {
 public:
  explicit StabilizerSimulation(const Circuit& circuit)
      : tableau_(circuit.n_qubits()),
        measurements_(collect_measurements(circuit)) {
    std::vector<std::size_t> wire(circuit.n_qubits());
    for (std::size_t i = 0; i < wire.size(); ++i) wire[i] = i;
    evolve(tableau_, circuit, wire);
  }

  std::string_view backend() const override { return "stabilizer"; }

  std::vector<int> sample(std::mt19937& rng) const override {
    // Measurement collapses the tableau, so work on a copy — each shot starts
    // from the evolved state.
    Tableau t = tableau_;
    std::vector<int> creg(measurements_.n_clbits, 0);
    std::vector<int> cache(t.n(), -1);  // outcome per qubit, measured once
    for (const MeasuredBit& m : measurements_.bits) {
      if (cache[m.qubit] < 0) cache[m.qubit] = t.measure(m.qubit, rng);
      creg[m.clbit] = cache[m.qubit];
    }
    return creg;
  }

  double expval(const std::string& pauli) const override {
    if (pauli.size() != tableau_.n()) {
      throw std::invalid_argument(
          "expval: Pauli string length must equal the number of qubits");
    }
    // Rotate P into a pure Z-string: H sends X->Z, (Sdg then H) sends Y->Z.
    Tableau t = tableau_;
    std::vector<std::size_t> support;
    for (std::size_t i = 0; i < tableau_.n(); ++i) {
      char p =
          pauli[tableau_.n() - 1 - i];  // right-to-left: rightmost = qubit 0
      if (p >= 'a' && p <= 'z') p = static_cast<char>(p - ('a' - 'A'));
      switch (p) {
        case 'I':
          break;
        case 'X':
          t.h(i);
          support.push_back(i);
          break;
        case 'Y':
          t.sdg(i);
          t.h(i);
          support.push_back(i);
          break;
        case 'Z':
          support.push_back(i);
          break;
        default:
          throw std::invalid_argument(
              std::string("expval: invalid Pauli character '") + p + "'");
      }
    }
    return t.z_string_expectation(support);
  }

 private:
  Tableau tableau_;
  Measurements measurements_;
};

}  // namespace

std::unique_ptr<Simulation> make_stabilizer_simulation(const Circuit& circuit) {
  return std::make_unique<StabilizerSimulation>(circuit);
}

}  // namespace ket
