// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
#pragma once

#include <cstddef>
#include <vector>

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

  const std::vector<Gate>& gates() const noexcept { return gates_; }

 private:
  std::size_t n_qubits_;
  std::vector<Gate> gates_;
};

}  // namespace ket
