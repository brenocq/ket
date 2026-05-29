// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
#pragma once

#include <complex>
#include <vector>

namespace ket {

class Circuit;

using Complex = std::complex<double>;
using StateVector = std::vector<Complex>;

StateVector run(const Circuit& circuit);

}  // namespace ket
