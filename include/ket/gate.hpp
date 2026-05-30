// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
#pragma once

#include <memory>
#include <string>
#include <vector>

#include <ket/qubit.hpp>

namespace ket {

class Circuit;  // forward declaration for composite-gate definitions

// clang-format off
enum class GateType {
  // Single-qubit gates
  H, X, Y, Z,
  S, Sdg, T, Tdg,
  Rx, Ry, Rz,
  U,  // general single-qubit unitary U(theta, phi, lambda)
  // Two-qubit gates
  CX, CZ, CP, Swap,
  // Structural operations (non-unitary / rendering only)
  Measure, Barrier, Composite, Probe,
};
// clang-format on

struct Gate {
  GateType type;
  std::vector<Qubit> qubits;  // for Composite: sub-qubit i maps to qubits[i]
  std::string label = "";     // Barrier label, or Composite block name
  // Non-null only for Composite: the sub-circuit this block expands to. Shared
  // so the same definition can back several appended blocks.
  std::shared_ptr<const Circuit> definition = nullptr;
  std::size_t clbit = 0;            // target classical bit, for Measure
  std::vector<double> params = {};  // gate parameters, e.g. a rotation angle
};

}  // namespace ket
