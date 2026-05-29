// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
#pragma once

#include <string>
#include <vector>

#include <ket/qubit.hpp>

namespace ket {

enum class GateType { H, X, Z, CNOT, Barrier };

struct Gate {
  GateType type;
  std::vector<Qubit> qubits;
  std::string label = "";  // used by Barrier; empty for unlabeled operations
};

}  // namespace ket
