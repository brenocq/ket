// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
//
// The dense state-vector backend: evolve the circuit into its 2^n amplitude
// vector once, then answer queries against it. This wraps the existing run() /
// measure() / expval(State, ...) so the default behavior is unchanged.
#include <cstddef>
#include <memory>
#include <random>
#include <string>
#include <string_view>
#include <vector>

#include <ket/backends/backend.hpp>
#include <ket/circuit.hpp>
#include <ket/measurement.hpp>
#include <ket/observable.hpp>
#include <ket/simulator.hpp>
#include <ket/state.hpp>

#include "internal.hpp"

namespace ket {
namespace {

class StateVectorSimulation final : public Simulation {
 public:
  explicit StateVectorSimulation(const Circuit& circuit)
      : state_(run(circuit)), measurements_(collect_measurements(circuit)) {}

  std::string_view backend() const override { return "state vector"; }

  std::vector<int> sample(std::mt19937& rng) const override {
    // One Born-rule draw collapses the whole register to a basis state; read
    // each measured qubit's bit out of that index.
    const std::size_t outcome = measure(state_, rng);
    std::vector<int> creg(measurements_.n_clbits, 0);
    for (const MeasuredBit& m : measurements_.bits) {
      creg[m.clbit] = static_cast<int>((outcome >> m.qubit) & 1u);
    }
    return creg;
  }

  double expval(const std::string& pauli) const override {
    return ket::expval(state_, pauli);
  }

 private:
  State state_;
  Measurements measurements_;
};

}  // namespace

std::unique_ptr<Simulation> make_statevector_simulation(
    const Circuit& circuit) {
  return std::make_unique<StateVectorSimulation>(circuit);
}

}  // namespace ket
