// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
#pragma once

#include <memory>
#include <string>
#include <vector>

#include <ket/qubit.hpp>

namespace ket {

class Circuit;  // forward declaration for composite-gate definitions

enum class GateType {
  H, X, Z, CNOT, Barrier, Composite, Measure, Rx, Ry, Rz, CZ, CP
};

struct Gate {
  GateType type;
  std::vector<Qubit> qubits;  // for Composite: sub-qubit i maps to qubits[i]
  std::string label = "";     // Barrier label, or Composite block name
  // Non-null only for Composite: the sub-circuit this block expands to. Shared
  // so the same definition can back several appended blocks.
  std::shared_ptr<const Circuit> definition = nullptr;
  std::size_t clbit = 0;             // target classical bit, for Measure
  std::vector<double> params = {};   // gate parameters, e.g. a rotation angle
};

}  // namespace ket
