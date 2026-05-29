// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
#pragma once

#include <complex>
#include <cstddef>
#include <string>
#include <vector>

namespace ket {

class Circuit;

using Complex = std::complex<double>;

// A quantum state vector: 2^n complex amplitudes over the computational basis.
class StateVector {
 public:
  StateVector() = default;
  StateVector(std::size_t size, Complex value) : amplitudes_(size, value) {}

  std::size_t size() const noexcept { return amplitudes_.size(); }

  Complex& operator[](std::size_t i) { return amplitudes_[i]; }
  const Complex& operator[](std::size_t i) const { return amplitudes_[i]; }

  auto begin() noexcept { return amplitudes_.begin(); }
  auto end() noexcept { return amplitudes_.end(); }
  auto begin() const noexcept { return amplitudes_.begin(); }
  auto end() const noexcept { return amplitudes_.end(); }

  std::string print() const;

 private:
  std::vector<Complex> amplitudes_;
};

StateVector run(const Circuit& circuit);

}  // namespace ket
