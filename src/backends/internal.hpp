// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
//
// Internal glue shared by the backend implementations. Not installed.
#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include <ket/backends/backend.hpp>

namespace ket {

class Circuit;

// A qubit measured into a classical bit by a Measure gate.
struct MeasuredBit {
  std::size_t qubit;
  std::size_t clbit;
};

// The classical register layout a sampled shot must fill.
struct Measurements {
  std::size_t n_clbits;
  std::vector<MeasuredBit> bits;
};

// Collect every Measure gate in the circuit (in circuit order).
Measurements collect_measurements(const Circuit& circuit);

// Backend constructors. Each evolves the circuit once and returns a Simulation.
std::unique_ptr<Simulation> make_statevector_simulation(const Circuit& circuit);
std::unique_ptr<Simulation> make_stabilizer_simulation(const Circuit& circuit);

}  // namespace ket
