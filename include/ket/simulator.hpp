// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
#pragma once

#include <complex>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace ket {

class Circuit;

using Complex = std::complex<double>;

// A quantum state vector: 2^n complex amplitudes over the computational basis.
class State {
 public:
  State() = default;
  State(std::size_t size, Complex value) : amplitudes_(size, value) {}

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

State run(const Circuit& circuit);

// The result of a probed run: the final state plus the states captured at each
// probe(), in circuit order.
struct ProbeRun {
  State final;
  std::vector<std::pair<std::string, State>> probes;
};

ProbeRun run_with_probes(const Circuit& circuit);

}  // namespace ket
