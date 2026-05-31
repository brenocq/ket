// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
#pragma once

#include <cstddef>
#include <random>
#include <vector>

#include <ket/backends/backend.hpp>
#include <ket/simulator.hpp>

namespace ket {

class Circuit;

std::size_t measure(const State& state, std::mt19937& rng);
std::size_t measure(const State& state);

// Run the circuit and sample one shot of its Measure operations. Returns the
// classical register: a vector of size circuit.n_clbits() where entry i is the
// sampled value of whichever qubit was measured into classical bit i. Any
// classical bit that no Measure writes to stays 0.
//
// `method` selects the backend: Auto (the default) uses the stabilizer engine
// for Clifford circuits and the dense state vector otherwise.
std::vector<int> sample(const Circuit& circuit, std::mt19937& rng,
                        Method method = Method::Auto);
std::vector<int> sample(const Circuit& circuit, Method method = Method::Auto);

}  // namespace ket
