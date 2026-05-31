// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
#pragma once

#include <cstddef>

#include <ket/circuit.hpp>
#include <ket/state.hpp>

namespace ket {

// Drives a circuit one gate at a time over a held state vector, for interactive
// step-through and debugging. Steps map one-to-one to the circuit's DAG nodes,
// so step k lines up with the k-th gate in the diagram; structural nodes
// (Barrier/Measure/Probe) advance the position without changing the state, and
// a composite block is applied as a single step. Always uses the dense state
// vector, so it is bounded by memory like run().
class Stepper {
 public:
  explicit Stepper(Circuit circuit);

  // Total number of steps (DAG nodes) and the current position in [0, size()].
  // state() reflects the first pos() gates applied to the ground state.
  std::size_t size() const noexcept { return n_steps_; }
  std::size_t pos() const noexcept { return pos_; }
  bool at_end() const noexcept { return pos_ == n_steps_; }

  void reset();  // back to |0...0>, pos() == 0
  bool step();   // apply the next gate; false if already at end
  // Jump to the state after the first `step` gates (clamped to size()).
  void seek(std::size_t step);

  const State& state() const noexcept { return state_; }
  const Circuit& circuit() const noexcept { return circuit_; }

 private:
  Circuit circuit_;
  State state_;
  std::size_t n_steps_;
  std::size_t pos_;
};

}  // namespace ket
