// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
#include <ket/observable.hpp>

#include <bit>
#include <complex>
#include <cstddef>
#include <stdexcept>
#include <string>

#include <ket/state.hpp>

namespace ket {

double expval(const State& state, const std::string& pauli) {
  const std::size_t dim = state.size();
  std::size_t n = 0;
  while ((std::size_t{1} << n) < dim) ++n;
  if ((std::size_t{1} << n) != dim) {
    throw std::invalid_argument("expval: state size is not a power of two");
  }
  if (pauli.size() != n) {
    throw std::invalid_argument(
        "expval: Pauli string length must equal the number of qubits");
  }

  // A Pauli string maps each basis state |x> to a phase times |x ^ flip>:
  //   - X and Y flip the qubit's bit (flip mask),
  //   - Z and Y contribute (-1)^bit (sign mask),
  //   - each Y also contributes a global factor i (collected as i^num_y).
  std::size_t flip = 0;
  std::size_t sign_mask = 0;
  int num_y = 0;
  for (std::size_t i = 0; i < n; ++i) {
    // Right-to-left: the rightmost character acts on qubit 0.
    char p = pauli[n - 1 - i];
    if (p >= 'a' && p <= 'z') p = static_cast<char>(p - ('a' - 'A'));
    const std::size_t bit = std::size_t{1} << i;
    switch (p) {
      case 'I':
        break;
      case 'X':
        flip |= bit;
        break;
      case 'Y':
        flip |= bit;
        sign_mask |= bit;
        ++num_y;
        break;
      case 'Z':
        sign_mask |= bit;
        break;
      default:
        throw std::invalid_argument(
            std::string("expval: invalid Pauli character '") + p + "'");
    }
  }

  Complex phase{1.0, 0.0};
  switch (num_y % 4) {
    case 1:
      phase = Complex{0.0, 1.0};
      break;
    case 2:
      phase = Complex{-1.0, 0.0};
      break;
    case 3:
      phase = Complex{0.0, -1.0};
      break;
    default:
      break;
  }

  Complex sum{0.0, 0.0};
  for (std::size_t x = 0; x < dim; ++x) {
    const Complex c = (std::popcount(x & sign_mask) % 2 == 0) ? phase : -phase;
    sum += c * state[x] * std::conj(state[x ^ flip]);
  }
  return sum.real();
}

}  // namespace ket
