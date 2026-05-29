// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
#include <ket/dag.hpp>

#include <algorithm>
#include <utility>

namespace ket {

Dag::Dag(std::size_t n_qubits) : wires_(n_qubits) {}

void Dag::add(Gate gate) {
  const std::size_t idx = nodes_.size();
  DagNode node{std::move(gate), {}, {}};
  for (const Qubit q : node.gate.qubits) {
    auto& wire = wires_[q.index];
    if (!wire.empty()) {
      const std::size_t prev = wire.back();
      const auto& preds = node.predecessors;
      if (std::find(preds.begin(), preds.end(), prev) == preds.end()) {
        node.predecessors.push_back(prev);
        nodes_[prev].successors.push_back(idx);
      }
    }
    wire.push_back(idx);
  }
  nodes_.push_back(std::move(node));
}

}  // namespace ket
