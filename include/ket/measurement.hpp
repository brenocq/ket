// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
#pragma once

#include <cstddef>
#include <random>
#include <vector>

#include <ket/simulator.hpp>

namespace ket {

class Circuit;

std::size_t measure(const StateVector& state, std::mt19937& rng);
std::size_t measure(const StateVector& state);

// Run the circuit and sample one shot of its Measure operations. Returns the
// classical register: a vector of size circuit.n_clbits() where entry i is the
// sampled value of whichever qubit was measured into classical bit i. Any
// classical bit that no Measure writes to stays 0.
std::vector<int> sample(const Circuit& circuit, std::mt19937& rng);
std::vector<int> sample(const Circuit& circuit);

}  // namespace ket
