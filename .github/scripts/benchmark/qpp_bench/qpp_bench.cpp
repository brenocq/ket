// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
//
// Benchmark harness for Quantum++: read an OpenQASM file, then time how long it
// takes to simulate it to its final state vector, averaged over `reps` runs.
// Prints the average wall-clock time (in seconds) to stdout.
//
//   qpp_bench <file.qasm> <reps>
#include <chrono>
#include <iostream>
#include <string>

#include <qpp/qpp.hpp>

int main(int argc, char** argv) {
  using namespace qpp;
  if (argc < 3) {
    std::cerr << "usage: qpp_bench <file.qasm> <reps>\n";
    return 1;
  }
  const std::string path = argv[1];
  const idx reps = static_cast<idx>(std::stoul(argv[2]));

  const QCircuit qc = qasm::read_from_file(path);  // parsing is not timed

  // Warm-up (discarded).
  {
    QEngine engine{qc};
    engine.execute();
  }

  double total = 0.0;
  for (idx i = 0; i < reps; ++i) {
    const auto start = std::chrono::steady_clock::now();
    QEngine engine{qc};  // allocate |0...0> and evolve it
    engine.execute();
    const auto end = std::chrono::steady_clock::now();
    total += std::chrono::duration<double>(end - start).count();
  }

  std::cout << (total / static_cast<double>(reps)) << "\n";
  return 0;
}
