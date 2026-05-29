// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
#include <ket/circuit.hpp>

#include <cassert>
#include <string>
#include <vector>

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

namespace {

// Each column is fixed at 5 display chars wide. Rows alternate:
// even rows are box-borders/separators, odd rows carry the qubit wire.
std::vector<std::string> default_column(std::size_t n_qubits) {
  const std::size_t height = 2 * n_qubits + 1;
  std::vector<std::string> col(height);
  for (std::size_t i = 0; i < height; ++i) {
    col[i] = (i % 2 == 1) ? "─────" : "     ";
  }
  return col;
}

std::vector<std::string> render_single(std::size_t n_qubits, std::size_t q,
                                       char label) {
  auto col = default_column(n_qubits);
  col[2 * q] = "┌───┐";
  col[2 * q + 1] = std::string("┤ ") + label + " ├";
  col[2 * q + 2] = "└───┘";
  return col;
}

std::vector<std::string> render_cnot(std::size_t n_qubits, std::size_t control,
                                     std::size_t target) {
  auto col = default_column(n_qubits);
  if (control < target) {
    col[2 * control + 1] = "──■──";
    for (std::size_t i = 2 * control + 2; i < 2 * target; ++i) {
      col[i] = (i % 2 == 1) ? "──┼──" : "  │  ";
    }
    col[2 * target] = "┌─┴─┐";
    col[2 * target + 1] = "┤ X ├";
    col[2 * target + 2] = "└───┘";
  } else {
    col[2 * target] = "┌───┐";
    col[2 * target + 1] = "┤ X ├";
    col[2 * target + 2] = "└─┬─┘";
    for (std::size_t i = 2 * target + 3; i <= 2 * control; ++i) {
      col[i] = (i % 2 == 1) ? "──┼──" : "  │  ";
    }
    col[2 * control + 1] = "──■──";
  }
  return col;
}

}  // namespace

std::string Circuit::print() const {
  if (n_qubits_ == 0) return "";
  const std::size_t height = 2 * n_qubits_ + 1;

  const std::size_t label_width =
      std::string("q_" + std::to_string(n_qubits_ - 1) + ":").size();
  std::vector<std::string> prefix(height, std::string(label_width + 1, ' '));
  for (std::size_t q = 0; q < n_qubits_; ++q) {
    std::string label = "q_" + std::to_string(q) + ":";
    label.resize(label_width, ' ');
    label += ' ';
    prefix[2 * q + 1] = std::move(label);
  }

  std::vector<std::vector<std::string>> columns;
  if (dag_.nodes().empty()) {
    columns.push_back(default_column(n_qubits_));
  }
  for (const DagNode& node : dag_.nodes()) {
    const Gate& g = node.gate;
    switch (g.type) {
      case GateType::H:
        columns.push_back(render_single(n_qubits_, g.qubits[0].index, 'H'));
        break;
      case GateType::X:
        columns.push_back(render_single(n_qubits_, g.qubits[0].index, 'X'));
        break;
      case GateType::Z:
        columns.push_back(render_single(n_qubits_, g.qubits[0].index, 'Z'));
        break;
      case GateType::CNOT:
        columns.push_back(render_cnot(n_qubits_, g.qubits[0].index,
                                      g.qubits[1].index));
        break;
    }
  }

  std::string result;
  for (std::size_t r = 0; r < height; ++r) {
    result += prefix[r];
    for (const auto& col : columns) {
      result += col[r];
    }
    result += '\n';
  }
  return result;
}

}  // namespace ket
