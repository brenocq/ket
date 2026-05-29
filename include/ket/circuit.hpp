// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
#pragma once

#include <cstddef>
#include <string>

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

  const Dag& dag() const noexcept { return dag_; }

  std::string print() const;

 private:
  std::size_t n_qubits_;
  Dag dag_;
};

}  // namespace ket
