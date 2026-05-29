// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
#include <ket/circuit.hpp>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <string>
#include <utility>
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

void Circuit::barrier(const std::string& label) {
  std::vector<Qubit> qubits;
  qubits.reserve(n_qubits_);
  for (std::size_t i = 0; i < n_qubits_; ++i) qubits.push_back(Qubit{i});
  dag_.add(Gate{GateType::Barrier, std::move(qubits), label});
}

void Circuit::barrier(std::initializer_list<std::size_t> qubits,
                      const std::string& label) {
  barrier(std::vector<std::size_t>(qubits), label);
}

void Circuit::barrier(const std::vector<std::size_t>& qubits,
                      const std::string& label) {
  std::vector<Qubit> qs;
  qs.reserve(qubits.size());
  for (std::size_t i : qubits) {
    assert(i < n_qubits_);
    qs.push_back(Qubit{i});
  }
  dag_.add(Gate{GateType::Barrier, std::move(qs), label});
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

// A barrier is a one-column-wide dashed vertical line (░) over the spanned
// qubits. Non-spanned wires still show the horizontal wire so they stay
// connected through the column.
std::vector<std::string> render_barrier(std::size_t n_qubits,
                                        const std::vector<Qubit>& qubits) {
  const std::size_t height = 2 * n_qubits + 1;
  std::vector<bool> spanned(n_qubits, false);
  std::size_t lo = n_qubits - 1;
  std::size_t hi = 0;
  for (const Qubit q : qubits) {
    spanned[q.index] = true;
    lo = std::min(lo, q.index);
    hi = std::max(hi, q.index);
  }
  std::vector<std::string> col(height);
  for (std::size_t r = 0; r < height; ++r) {
    if (r % 2 == 1) {
      col[r] = spanned[(r - 1) / 2] ? "░" : "─";
    } else {
      col[r] = (r >= 2 * lo && r <= 2 * hi + 2) ? "░" : " ";
    }
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

  const std::size_t prefix_width = label_width + 1;
  std::vector<std::vector<std::string>> columns;
  std::vector<std::size_t> widths;  // display width of each column
  // Top-row annotations for labeled barriers: (display offset, text).
  std::vector<std::pair<std::size_t, std::string>> top_labels;

  auto add_column = [&](std::vector<std::string> col, std::size_t width) {
    columns.push_back(std::move(col));
    widths.push_back(width);
  };
  auto current_offset = [&]() {
    std::size_t offset = prefix_width;
    for (std::size_t w : widths) offset += w;
    return offset;
  };

  if (dag_.nodes().empty()) {
    add_column(default_column(n_qubits_), 5);
  }
  for (const DagNode& node : dag_.nodes()) {
    const Gate& g = node.gate;
    switch (g.type) {
      case GateType::H:
        add_column(render_single(n_qubits_, g.qubits[0].index, 'H'), 5);
        break;
      case GateType::X:
        add_column(render_single(n_qubits_, g.qubits[0].index, 'X'), 5);
        break;
      case GateType::Z:
        add_column(render_single(n_qubits_, g.qubits[0].index, 'Z'), 5);
        break;
      case GateType::CNOT:
        add_column(
            render_cnot(n_qubits_, g.qubits[0].index, g.qubits[1].index), 5);
        break;
      case GateType::Barrier:
        if (!g.label.empty()) top_labels.emplace_back(current_offset(), g.label);
        add_column(render_barrier(n_qubits_, g.qubits), 1);
        break;
    }
  }

  std::string result;

  // A label row is emitted only when a barrier carries a label, so unlabeled
  // circuits render exactly as before. The row is pure ASCII (spaces + labels),
  // so byte offsets line up with the display columns of the grid below.
  if (!top_labels.empty()) {
    std::string row;
    for (const auto& [offset, text] : top_labels) {
      if (row.size() < offset) row.resize(offset, ' ');
      for (std::size_t k = 0; k < text.size(); ++k) {
        if (offset + k < row.size()) {
          row[offset + k] = text[k];
        } else {
          row.push_back(text[k]);
        }
      }
    }
    result += row;
    result += '\n';
  }

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
