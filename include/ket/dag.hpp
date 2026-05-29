// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
#pragma once

#include <cstddef>
#include <vector>

#include <ket/gate.hpp>

namespace ket {

struct DagNode {
  Gate gate;
  std::vector<std::size_t> predecessors;
  std::vector<std::size_t> successors;
};

class Dag {
 public:
  explicit Dag(std::size_t n_qubits);

  std::size_t n_qubits() const noexcept { return wires_.size(); }
  const std::vector<DagNode>& nodes() const noexcept { return nodes_; }
  const std::vector<std::size_t>& wire(std::size_t qubit) const {
    return wires_[qubit];
  }

  void add(Gate gate);

 private:
  std::vector<DagNode> nodes_;
  std::vector<std::vector<std::size_t>> wires_;
};

}  // namespace ket
