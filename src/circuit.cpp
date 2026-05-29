// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
#include <ket/circuit.hpp>

#include <cassert>

namespace ket {

Circuit::Circuit(std::size_t n_qubits) : n_qubits_(n_qubits), dag_(n_qubits) {}

Qubit Circuit::qubit(std::size_t i) const {
  assert(i < n_qubits_);
  return Qubit{i};
}

void Circuit::h(Qubit q) {
  assert(q.index < n_qubits_);
  dag_.add(Gate{GateType::H, {q}});
}

void Circuit::x(Qubit q) {
  assert(q.index < n_qubits_);
  dag_.add(Gate{GateType::X, {q}});
}

void Circuit::z(Qubit q) {
  assert(q.index < n_qubits_);
  dag_.add(Gate{GateType::Z, {q}});
}

void Circuit::cnot(Qubit control, Qubit target) {
  assert(control.index < n_qubits_);
  assert(target.index < n_qubits_);
  assert(control.index != target.index);
  dag_.add(Gate{GateType::CNOT, {control, target}});
}

}  // namespace ket
