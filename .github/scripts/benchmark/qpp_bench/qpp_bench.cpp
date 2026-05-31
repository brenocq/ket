// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
//
// Benchmark harness for Quantum++: read an OpenQASM file, then either time how
// long it takes to simulate it to its final state vector, or print that state.
//
//   qpp_bench <file.qasm> <reps>      time `reps` runs; print one second-count
//                                     per run, space-separated, on one line
//   qpp_bench <file.qasm> --state     print the final state vector, one
//                                     "<real> <imag>" amplitude per line
#include <chrono>
#include <iostream>
#include <string>

#include <qpp/qpp.hpp>

int main(int argc, char** argv) {
  using namespace qpp;
  if (argc < 3) {
    std::cerr << "usage: qpp_bench <file.qasm> (<reps> | --state)\n";
    return 1;
  }
  const std::string path = argv[1];
  const std::string mode = argv[2];

  const QCircuit qc = qasm::read_from_file(path);  // parsing is not timed

  if (mode == "--state") {
    QEngine engine{qc};
    engine.execute();
    const ket psi = engine.get_state();
    std::cout.precision(17);
    for (idx i = 0; i < static_cast<idx>(psi.size()); ++i) {
      std::cout << psi[i].real() << " " << psi[i].imag() << "\n";
    }
    return 0;
  }

  const idx reps = static_cast<idx>(std::stoul(mode));

  // Warm-up (discarded).
  {
    QEngine engine{qc};
    engine.execute();
  }

  for (idx i = 0; i < reps; ++i) {
    const auto start = std::chrono::steady_clock::now();
    QEngine engine{qc};  // allocate |0...0> and evolve it
    engine.execute();
    const auto end = std::chrono::steady_clock::now();
    if (i) std::cout << " ";
    std::cout << std::chrono::duration<double>(end - start).count();
  }
  std::cout << "\n";
  return 0;
}
