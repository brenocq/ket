// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
//
// Three-qubit bit-flip code — the simplest quantum error-correcting code. A
// logical qubit is spread across three data qubits (|0⟩_L = |000⟩,
// |1⟩_L = |111⟩). If a single X (bit-flip) error strikes one of them, two
// ancilla qubits measure parity "syndromes" that pinpoint which qubit flipped —
// crucially, without measuring (and thus collapsing) the logical data itself.
//
// Syndrome s0 = parity(data0, data1), s1 = parity(data1, data2):
//   00 -> no error      10 -> data0 flipped
//   11 -> data1 flipped  01 -> data2 flipped
#include <array>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include <ket/ket.hpp>

namespace {

constexpr std::size_t kData0 = 0;
constexpr std::size_t kData1 = 1;
constexpr std::size_t kData2 = 2;
constexpr std::size_t kAnc0 = 3;  // syndrome ancillas
constexpr std::size_t kAnc1 = 4;
constexpr std::size_t kQubits = 5;

constexpr int kNoError = -1;

// Spread the data qubit across two others: |ψ⟩|00⟩ -> α|000⟩ + β|111⟩.
ket::Circuit encode_block() {
  ket::Circuit enc{3, "encode"};
  enc.cnot(0, 1);
  enc.cnot(0, 2);
  return enc;
}

ket::Circuit bit_flip_code(int error_qubit) {
  ket::Circuit c{kQubits};
  c.h(kData0);  // logical |+⟩, so the data is a genuine superposition
  c.append(encode_block(), {kData0, kData1, kData2});

  c.barrier("error");
  if (error_qubit != kNoError) c.x(static_cast<std::size_t>(error_qubit));

  c.barrier("syndrome");
  c.cnot(kData0, kAnc0);  // ancilla 0 = parity(data0, data1)
  c.cnot(kData1, kAnc0);
  c.cnot(kData1, kAnc1);  // ancilla 1 = parity(data1, data2)
  c.cnot(kData2, kAnc1);
  c.measure(kAnc0, 0);
  c.measure(kAnc1, 1);
  return c;
}

// Map a syndrome (s0, s1) back to the flipped qubit (-1 = none).
int detect(int s0, int s1) {
  if (s0 == 0 && s1 == 0) return kNoError;
  if (s0 == 1 && s1 == 0) return 0;
  if (s0 == 1 && s1 == 1) return 1;
  return 2;  // (0, 1)
}

std::string label(int qubit) {
  return qubit < 0 ? "none" : "data qubit " + std::to_string(qubit);
}

}  // namespace

int main() {
  std::cout << "Three-qubit bit-flip code.\n"
            << "Encodes one logical qubit into three; a 2-bit syndrome locates "
               "a single X error\nwithout disturbing the encoded data.\n\n";

  // Show the circuit for an error on data qubit 1.
  ket::Circuit demo = bit_flip_code(1);
  std::cout << "Example circuit (X error on data qubit 1):\n"
            << demo.print() << '\n';

  // The syndrome is deterministic and independent of the (superposed) logical
  // state, so a single shot decides it.
  std::cout << "Syndrome (s0 s1) for each single-qubit error:\n";
  const std::array<int, 4> errors{kNoError, 0, 1, 2};
  for (int e : errors) {
    const std::vector<int> creg = ket::sample(bit_flip_code(e));
    const int detected = detect(creg[0], creg[1]);
    std::cout << "  injected " << std::left << std::setw(13) << label(e)
              << " -> syndrome " << creg[0] << creg[1] << "  => detected "
              << std::setw(13) << label(detected) << " ["
              << (detected == e ? "OK" : "WRONG") << "]\n";
  }

  return 0;
}
