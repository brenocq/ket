// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
//
// Multithreading the state-vector kernels must not change the result: every
// amplitude is computed by exactly one thread, with the same arithmetic, so the
// parallel state must match the serial one.
#include <gtest/gtest.h>
#include <ket/circuit.hpp>
#include <ket/parallelism.hpp>
#include <ket/simulator.hpp>
#include <ket/state.hpp>

#include <cmath>
#include <cstddef>
#include <random>

namespace {

// A wide random circuit (>= 2^14 amplitudes, so the parallel path engages)
// exercising single-qubit, controlled, swap, and diagonal kernels.
ket::Circuit random_mixed(std::size_t n, int depth, std::mt19937& rng) {
  ket::Circuit c{n};
  std::uniform_int_distribution<int> gate(0, 8);
  std::uniform_int_distribution<std::size_t> qubit(0, n - 1);
  std::uniform_real_distribution<double> angle(0.0, 6.283185307179586);
  for (int d = 0; d < depth; ++d) {
    const std::size_t a = qubit(rng);
    std::size_t b = qubit(rng);
    while (b == a) b = qubit(rng);
    switch (gate(rng)) {
      case 0:
        c.h(a);
        break;
      case 1:
        c.rx(a, angle(rng));
        break;
      case 2:
        c.ry(a, angle(rng));
        break;
      case 3:
        c.t(a);
        break;
      case 4:
        c.cx(a, b);
        break;
      case 5:
        c.cz(a, b);
        break;
      case 6:
        c.swap(a, b);
        break;
      case 7:
        c.cp(a, b, angle(rng));
        break;
      case 8:
        c.cy(a, b);
        break;
    }
  }
  return c;
}

ket::State run_with_threads(const ket::Circuit& c, unsigned threads) {
  ket::set_num_threads(threads);
  return ket::run(c);
}

}  // namespace

TEST(Parallelism, ParallelMatchesSerial) {
  std::mt19937 rng{2024};
  for (int trial = 0; trial < 5; ++trial) {
    const ket::Circuit c = random_mixed(16, 200, rng);  // 65536 amplitudes
    const ket::State serial = run_with_threads(c, 1);
    for (unsigned threads : {2u, 4u, 8u}) {
      const ket::State parallel = run_with_threads(c, threads);
      ASSERT_EQ(serial.size(), parallel.size());
      for (std::size_t i = 0; i < serial.size(); ++i) {
        EXPECT_DOUBLE_EQ(serial[i].real(), parallel[i].real()) << "i=" << i;
        EXPECT_DOUBLE_EQ(serial[i].imag(), parallel[i].imag()) << "i=" << i;
      }
    }
  }
  ket::set_num_threads(1);
}

// Circuits below the parallelization threshold must also stay correct.
TEST(Parallelism, SmallCircuitWithThreads) {
  ket::set_num_threads(8);
  ket::Circuit c{2};
  c.h(0);
  c.cx(0, 1);  // Bell state
  const ket::State s = ket::run(c);
  const double inv_sqrt2 = 1.0 / std::sqrt(2.0);
  EXPECT_NEAR(s[0].real(), inv_sqrt2, 1e-12);
  EXPECT_NEAR(s[3].real(), inv_sqrt2, 1e-12);
  EXPECT_NEAR(std::abs(s[1]), 0.0, 1e-12);
  EXPECT_NEAR(std::abs(s[2]), 0.0, 1e-12);
  ket::set_num_threads(1);
}

TEST(Parallelism, ThreadCountApi) {
  ket::set_num_threads(4);
  EXPECT_EQ(ket::num_threads(), 4u);
  ket::set_num_threads(0);  // hardware concurrency
  EXPECT_GE(ket::num_threads(), 1u);
  ket::set_num_threads(1);
  EXPECT_EQ(ket::num_threads(), 1u);
}
