// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
#pragma once

#include <memory>
#include <random>
#include <string>
#include <string_view>
#include <vector>

namespace ket {

class Circuit;

// How to simulate a circuit. `Auto` inspects the circuit and picks the fastest
// backend that can handle it (see chosen_method); the others force a specific
// engine and throw if it can't represent the circuit.
enum class Method { Auto, StateVector, Stabilizer };

// A circuit that has already been evolved on some backend. Its results are
// representation-agnostic — samples and Pauli expectation values — so callers
// never learn (or care) which engine produced them. There is deliberately no
// way to ask a Simulation for the full 2^n state vector: that operation is
// inherently dense and lives in run() instead, so the stabilizer backend is
// never forced to pay an exponential cost it was meant to avoid.
class Simulation {
 public:
  virtual ~Simulation() = default;

  // Human-readable backend name, e.g. "state vector" or "stabilizer".
  virtual std::string_view backend() const = 0;

  // Sample one shot of the circuit's measurements, returning the classical
  // register (see sample() in measurement.hpp). Cheap to call repeatedly: the
  // circuit is evolved once, at construction.
  virtual std::vector<int> sample(std::mt19937& rng) const = 0;

  // Expectation value <psi|P|psi> of a Pauli-string observable (same right-to-
  // left convention as expval() in observable.hpp).
  virtual double expval(const std::string& pauli) const = 0;
};

// Whether `circuit` uses only Clifford gates (H, X, Y, Z, S, Sdg, CX, CY, CZ,
// SWAP), recursing into composite blocks. Structural ops (measure, barrier,
// probe) are transparent. This is the gate-set test that gates the stabilizer
// backend, and it runs in a single O(#gates) pass.
bool is_clifford(const Circuit& circuit);

// The backend Auto would choose for `circuit`, without simulating — Stabilizer
// for Clifford circuits, StateVector otherwise.
Method chosen_method(const Circuit& circuit);

// Evolve `circuit` on the chosen backend and return a queryable Simulation.
// `Auto` resolves via chosen_method(); an explicit Method that cannot represent
// the circuit throws std::invalid_argument.
std::unique_ptr<Simulation> simulate(const Circuit& circuit,
                                     Method method = Method::Auto);

}  // namespace ket
