// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
#include <ket/state.hpp>

#include <cstddef>
#include <sstream>
#include <string>

namespace ket {
namespace {

std::string ket_label(std::size_t index, std::size_t n_qubits) {
  std::string bits(n_qubits, '0');
  for (std::size_t b = 0; b < n_qubits; ++b) {
    if ((index >> b) & 1u) bits[n_qubits - 1 - b] = '1';
  }
  return "|" + bits + "⟩";  // |bits⟩
}

std::string format_amplitude(Complex z) {
  std::ostringstream os;
  if (z.imag() == 0.0) {
    os << z.real();
  } else if (z.real() == 0.0) {
    os << z.imag() << "i";
  } else if (z.imag() < 0.0) {
    os << z.real() << " - " << -z.imag() << "i";
  } else {
    os << z.real() << " + " << z.imag() << "i";
  }
  return os.str();
}

std::size_t qubit_count(std::size_t dim) {
  std::size_t n = 0;
  while ((std::size_t{1} << n) < dim) ++n;
  return n;
}

}  // namespace

std::string State::print() const {
  const std::size_t n = qubit_count(size());
  std::string result;
  for (std::size_t i = 0; i < size(); ++i) {
    result += ket_label(i, n);
    result += ": ";
    result += format_amplitude(amplitudes_[i]);
    result += '\n';
  }
  return result;
}

}  // namespace ket
