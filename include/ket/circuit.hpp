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

  void h(Qubit q);
  void x(Qubit q);
  void y(Qubit q);
  void z(Qubit q);
  void s(Qubit q);    // phase gate (sqrt Z): diag(1, i)
  void sdg(Qubit q);  // S dagger: diag(1, -i)
  void t(Qubit q);    // T gate: diag(1, e^{i pi/4})
  void tdg(Qubit q);  // T dagger: diag(1, e^{-i pi/4})
  void cnot(Qubit control, Qubit target);

  // Single-qubit rotations by angle theta (radians) about the x/y/z axes.
  void rx(Qubit q, double theta);
  void ry(Qubit q, double theta);
  void rz(Qubit q, double theta);

  // Controlled-Z, and controlled-phase by angle lambda (CZ = cp with
  // lambda=pi). Both are symmetric in their two qubits.
  void cz(Qubit a, Qubit b);
  void cp(Qubit a, Qubit b, double lambda);

  void h(std::size_t i) { h(Qubit{i}); }
  void x(std::size_t i) { x(Qubit{i}); }
  void y(std::size_t i) { y(Qubit{i}); }
  void z(std::size_t i) { z(Qubit{i}); }
  void s(std::size_t i) { s(Qubit{i}); }
  void sdg(std::size_t i) { sdg(Qubit{i}); }
  void t(std::size_t i) { t(Qubit{i}); }
  void tdg(std::size_t i) { tdg(Qubit{i}); }
  void cnot(std::size_t control, std::size_t target) {
    cnot(Qubit{control}, Qubit{target});
  }
  void rx(std::size_t i, double theta) { rx(Qubit{i}, theta); }
  void ry(std::size_t i, double theta) { ry(Qubit{i}, theta); }
  void rz(std::size_t i, double theta) { rz(Qubit{i}, theta); }
  void cz(std::size_t a, std::size_t b) { cz(Qubit{a}, Qubit{b}); }
  void cp(std::size_t a, std::size_t b, double lambda) {
    cp(Qubit{a}, Qubit{b}, lambda);
  }

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
