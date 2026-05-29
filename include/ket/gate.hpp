// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
#pragma once

#include <vector>

#include <ket/qubit.hpp>

namespace ket {

enum class GateType { H, X, Z, CNOT };

struct Gate {
  GateType type;
  std::vector<Qubit> qubits;
};

}  // namespace ket
