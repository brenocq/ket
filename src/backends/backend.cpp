// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
#include <ket/backends/backend.hpp>

#include <stdexcept>
#include <string>

#include <ket/circuit.hpp>
#include <ket/dag.hpp>
#include <ket/gate.hpp>

#include "internal.hpp"

namespace ket {

bool is_clifford(const Circuit& circuit) {
  for (const DagNode& node : circuit.dag().nodes()) {
    switch (node.gate.type) {
      // The Clifford gate set ket can route to the stabilizer backend.
      case GateType::H:
      case GateType::X:
      case GateType::Y:
      case GateType::Z:
      case GateType::S:
      case GateType::Sdg:
      case GateType::CX:
      case GateType::CY:
      case GateType::CZ:
      case GateType::Swap:
      // Structural ops do not affect Clifford-ness.
      case GateType::Measure:
      case GateType::Barrier:
      case GateType::Probe:
        break;
      case GateType::Composite:
        if (!node.gate.definition || !is_clifford(*node.gate.definition)) {
          return false;
        }
        break;
      // Everything else (T, rotations, U, CH, controlled rotations, CP, CCX,
      // CSwap) is non-Clifford.
      default:
        return false;
    }
  }
  return true;
}

Method chosen_method(const Circuit& circuit) {
  if (is_clifford(circuit)) return Method::Stabilizer;
  return Method::StateVector;
}

std::unique_ptr<Simulation> simulate(const Circuit& circuit, Method method) {
  if (method == Method::Auto) method = chosen_method(circuit);
  switch (method) {
    case Method::StateVector:
      return make_statevector_simulation(circuit);
    case Method::Stabilizer:
      if (!is_clifford(circuit)) {
        throw std::invalid_argument(
            "stabilizer backend requires a Clifford circuit (it contains a "
            "non-Clifford gate); use Method::Auto or Method::StateVector");
      }
      return make_stabilizer_simulation(circuit);
    case Method::Auto:
      break;  // resolved above
  }
  throw std::logic_error("simulate: unreachable method");
}

Measurements collect_measurements(const Circuit& circuit) {
  Measurements out{circuit.n_clbits(), {}};
  for (const DagNode& node : circuit.dag().nodes()) {
    if (node.gate.type == GateType::Measure) {
      out.bits.push_back({node.gate.qubits[0].index, node.gate.clbit});
    }
  }
  return out;
}

}  // namespace ket
