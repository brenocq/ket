// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
#pragma once

#include <cstddef>
#include <initializer_list>
#include <string>
#include <vector>

#include <ket/dag.hpp>
#include <ket/gate.hpp>
#include <ket/qubit.hpp>

namespace ket {

class Circuit {
 public:
  explicit Circuit(std::size_t n_qubits, std::string name = "");

  std::size_t n_qubits() const noexcept { return n_qubits_; }
  std::size_t n_clbits() const noexcept { return n_clbits_; }
  const std::string& name() const noexcept { return name_; }
  Qubit qubit(std::size_t i) const;

  // Gates, in the same order as the GateType enum. These take Qubit handles;
  // size_t overloads (the c.h(0) shorthand) follow further below.

  // Hadamard and the Paulis.
  void h(Qubit q);
  void x(Qubit q);
  void y(Qubit q);
  void z(Qubit q);

  // Phase gates: S = diag(1, i), T = diag(1, e^{i pi/4}); sdg/tdg invert them.
  void s(Qubit q);
  void sdg(Qubit q);
  void t(Qubit q);
  void tdg(Qubit q);

  // Single-qubit rotations by theta (radians) about the x/y/z axes.
  void rx(Qubit q, double theta);
  void ry(Qubit q, double theta);
  void rz(Qubit q, double theta);

  // General single-qubit unitary (the OpenQASM U gate); u1/u2/u3 are the cases
  // u(0,0,lambda), u(pi/2,phi,lambda), and u(theta,phi,lambda).
  void u(Qubit q, double theta, double phi, double lambda);

  // Two-qubit gates: ch, cx (CNOT), and cy are controlled (control, target);
  // cz, cp (controlled-phase), and swap are symmetric in their qubits.
  void ch(Qubit control, Qubit target);
  void cx(Qubit control, Qubit target);
  void cy(Qubit control, Qubit target);
  void cz(Qubit a, Qubit b);
  void cp(Qubit a, Qubit b, double lambda);
  void swap(Qubit a, Qubit b);

  // size_t overloads of the gates above.
  void h(std::size_t i) { h(Qubit{i}); }
  void x(std::size_t i) { x(Qubit{i}); }
  void y(std::size_t i) { y(Qubit{i}); }
  void z(std::size_t i) { z(Qubit{i}); }
  void s(std::size_t i) { s(Qubit{i}); }
  void sdg(std::size_t i) { sdg(Qubit{i}); }
  void t(std::size_t i) { t(Qubit{i}); }
  void tdg(std::size_t i) { tdg(Qubit{i}); }
  void rx(std::size_t i, double theta) { rx(Qubit{i}, theta); }
  void ry(std::size_t i, double theta) { ry(Qubit{i}, theta); }
  void rz(std::size_t i, double theta) { rz(Qubit{i}, theta); }
  void u(std::size_t i, double theta, double phi, double lambda) {
    u(Qubit{i}, theta, phi, lambda);
  }
  void ch(std::size_t control, std::size_t target) {
    ch(Qubit{control}, Qubit{target});
  }
  void cx(std::size_t control, std::size_t target) {
    cx(Qubit{control}, Qubit{target});
  }
  void cy(std::size_t control, std::size_t target) {
    cy(Qubit{control}, Qubit{target});
  }
  void cz(std::size_t a, std::size_t b) { cz(Qubit{a}, Qubit{b}); }
  void cp(std::size_t a, std::size_t b, double lambda) {
    cp(Qubit{a}, Qubit{b}, lambda);
  }
  void swap(std::size_t a, std::size_t b) { swap(Qubit{a}, Qubit{b}); }

  // A barrier across all qubits, optionally labeled. Barriers are no-ops in
  // simulation but appear in the diagram and serialize the DAG across them.
  void barrier(const std::string& label = "");
  // A barrier across a subset of qubits (e.g. c.barrier({0, 1}, "prep")).
  // The initializer-list form keeps the braced syntax unambiguous; the vector
  // form is for programmatic lists (and the Python binding).
  void barrier(std::initializer_list<std::size_t> qubits,
               const std::string& label = "");
  void barrier(const std::vector<std::size_t>& qubits,
               const std::string& label = "");

  // Measure a qubit into a classical bit; the classical register grows to fit.
  void measure(std::size_t qubit, std::size_t clbit);
  // Measure every qubit i into classical bit i.
  void measure_all();

  // Mark a point to capture the state vector (see run_with_probes). It is a
  // no-op for an ordinary run; an empty label auto-numbers as ψ0, ψ1, ...
  void probe(const std::string& label = "");

  // Append a sub-circuit as a single composite block acting on the given parent
  // qubits (sub-qubit i -> qubits[i]). It renders as one labeled box and can be
  // expanded with decompose(). `name` defaults to the sub-circuit's name.
  void append(const Circuit& sub, const std::vector<std::size_t>& qubits,
              const std::string& name = "");

  // Return a copy with every composite block expanded one level into its
  // definition (with qubits remapped). Primitive gates are copied unchanged.
  Circuit decompose() const;

  const Dag& dag() const noexcept { return dag_; }

  std::string print() const;

 private:
  std::size_t n_qubits_;
  std::size_t n_clbits_ = 0;
  std::string name_;
  Dag dag_;
};

}  // namespace ket
