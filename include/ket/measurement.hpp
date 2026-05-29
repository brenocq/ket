// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
#pragma once

#include <cstddef>
#include <random>

#include <ket/simulator.hpp>

namespace ket {

std::size_t measure(const StateVector& state, std::mt19937& rng);
std::size_t measure(const StateVector& state);

}  // namespace ket
