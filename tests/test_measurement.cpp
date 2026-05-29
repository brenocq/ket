#include <gtest/gtest.h>
#include <ket/circuit.hpp>
#include <ket/measurement.hpp>
#include <ket/simulator.hpp>

#include <array>
#include <random>

TEST(Measurement, GroundStateAlwaysZero) {
  ket::Circuit c{2};
  auto s = ket::run(c);
  std::mt19937 rng{42};
  for (int i = 0; i < 100; ++i) {
    EXPECT_EQ(ket::measure(s, rng), 0u);
  }
}

TEST(Measurement, AfterXAlwaysOne) {
  ket::Circuit c{1};
  c.x(0);
  auto s = ket::run(c);
  std::mt19937 rng{42};
  for (int i = 0; i < 100; ++i) {
    EXPECT_EQ(ket::measure(s, rng), 1u);
  }
}

TEST(Measurement, BellCircuitOnlyCorrelatedOutcomes) {
  ket::Circuit c{2};
  c.h(0);
  c.cnot(0, 1);
  auto s = ket::run(c);

  std::mt19937 rng{42};
  std::array<int, 4> counts{0, 0, 0, 0};
  constexpr int shots = 10000;
  for (int i = 0; i < shots; ++i) {
    ++counts[ket::measure(s, rng)];
  }

  // |01> and |10> must never occur (perfect correlation).
  EXPECT_EQ(counts[1], 0);
  EXPECT_EQ(counts[2], 0);

  // |00> and |11> should each be ~5000. 4σ ≈ ±200.
  EXPECT_GT(counts[0], 4800);
  EXPECT_LT(counts[0], 5200);
  EXPECT_GT(counts[3], 4800);
  EXPECT_LT(counts[3], 5200);
  EXPECT_EQ(counts[0] + counts[3], shots);
}

TEST(Measurement, NoRngOverloadSamplesValidOutcome) {
  ket::Circuit c{1};
  c.h(0);
  auto s = ket::run(c);
  for (int i = 0; i < 100; ++i) {
    const auto out = ket::measure(s);
    EXPECT_TRUE(out == 0u || out == 1u);
  }
}

TEST(Measurement, HadamardGivesFiftyFifty) {
  ket::Circuit c{1};
  c.h(0);
  auto s = ket::run(c);

  std::mt19937 rng{42};
  int zeros = 0;
  int ones = 0;
  constexpr int shots = 10000;
  for (int i = 0; i < shots; ++i) {
    const auto out = ket::measure(s, rng);
    if (out == 0u) ++zeros;
    else if (out == 1u) ++ones;
    else FAIL() << "unexpected outcome " << out;
  }
  EXPECT_GT(zeros, 4800);
  EXPECT_LT(zeros, 5200);
  EXPECT_EQ(zeros + ones, shots);
}
