// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
//
// The Stepper evolves a circuit one DAG node at a time over a held state. Its
// final state must match run(), positions must track stepping, and seeking
// backward (which replays from the ground state) must be consistent.
#include <gtest/gtest.h>
#include <ket/circuit.hpp>
#include <ket/simulator.hpp>
#include <ket/state.hpp>
#include <ket/stepper.hpp>

#include <cmath>
#include <cstddef>

namespace {

// A small non-Clifford circuit with a structural node (barrier) in the middle.
ket::Circuit sample_circuit() {
  ket::Circuit c{3};
  c.h(0);
  c.cx(0, 1);
  c.rx(2, 0.7);
  c.barrier();
  c.cz(1, 2);
  c.t(0);
  c.ccx(0, 1, 2);
  return c;
}

void expect_states_near(const ket::State& a, const ket::State& b) {
  ASSERT_EQ(a.size(), b.size());
  for (std::size_t i = 0; i < a.size(); ++i) {
    EXPECT_NEAR(a[i].real(), b[i].real(), 1e-12) << "amp " << i;
    EXPECT_NEAR(a[i].imag(), b[i].imag(), 1e-12) << "amp " << i;
  }
}

}  // namespace

TEST(Stepper, StartsInGroundState) {
  ket::Stepper s{sample_circuit()};
  EXPECT_EQ(s.pos(), 0u);
  EXPECT_EQ(s.size(), s.circuit().dag().nodes().size());
  ASSERT_EQ(s.state().size(), std::size_t{1} << 3);
  EXPECT_NEAR(std::abs(s.state()[0]), 1.0, 1e-12);
  for (std::size_t i = 1; i < s.state().size(); ++i)
    EXPECT_NEAR(std::abs(s.state()[i]), 0.0, 1e-12);
}

TEST(Stepper, SteppingToEndMatchesRun) {
  const ket::Circuit c = sample_circuit();
  ket::Stepper s{c};
  std::size_t steps = 0;
  while (s.step()) ++steps;
  EXPECT_EQ(steps, s.size());
  EXPECT_TRUE(s.at_end());
  EXPECT_FALSE(s.step());  // no-op past the end
  expect_states_near(s.state(), ket::run(c));
}

TEST(Stepper, SeekForwardEqualsIncrementalStepping) {
  const ket::Circuit c = sample_circuit();
  ket::Stepper a{c};
  ket::Stepper b{c};
  for (std::size_t k = 0; k <= a.size(); ++k) {
    a.seek(k);
    EXPECT_EQ(a.pos(), k);
    b.reset();
    for (std::size_t j = 0; j < k; ++j) b.step();
    expect_states_near(a.state(), b.state());
  }
}

TEST(Stepper, SeekBackwardReplaysConsistently) {
  ket::Stepper s{sample_circuit()};
  s.seek(s.size());
  for (std::size_t k = s.size() + 1; k-- > 0;) {
    s.seek(k);
    EXPECT_EQ(s.pos(), k);
    ket::Stepper fresh{sample_circuit()};
    fresh.seek(k);
    expect_states_near(s.state(), fresh.state());
  }
}

TEST(Stepper, BarrierIsANoOpStep) {
  ket::Circuit c{1};
  c.h(0);
  c.barrier();
  ket::Stepper s{c};
  s.step();  // H
  const ket::State after_h = s.state();
  s.step();  // barrier — must not change the state
  expect_states_near(s.state(), after_h);
}
