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
  explicit Circuit(std::size_t n_qubits);

  std::size_t n_qubits() const noexcept { return n_qubits_; }
  Qubit qubit(std::size_t i) const;

  void h(Qubit q);
  void x(Qubit q);
  void z(Qubit q);
  void cnot(Qubit control, Qubit target);

  void h(std::size_t i) { h(Qubit{i}); }
  void x(std::size_t i) { x(Qubit{i}); }
  void z(std::size_t i) { z(Qubit{i}); }
  void cnot(std::size_t control, std::size_t target) {
    cnot(Qubit{control}, Qubit{target});
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

  const Dag& dag() const noexcept { return dag_; }

  std::string print() const;

 private:
  std::size_t n_qubits_;
  Dag dag_;
};

}  // namespace ket
