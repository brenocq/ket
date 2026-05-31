// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
//
// Gate fusion (engaged at >= 2^20 amplitudes) must not change the result. A
// barrier breaks a fusion block, so the same gates with a barrier after each
// apply one at a time through the specialized kernels — the reference the fused
// matrix path must match.
#include <gtest/gtest.h>
#include <ket/circuit.hpp>
#include <ket/parallelism.hpp>
#include <ket/simulator.hpp>
#include <ket/state.hpp>

#include <cmath>
#include <cstddef>
#include <random>
#include <vector>

namespace {

struct Op {
  int kind;
  std::size_t a;
  std::size_t b;
  double theta;
};

std::vector<Op> random_ops(std::size_t n, int count, std::mt19937& rng) {
  std::uniform_int_distribution<int> kind(0, 4);
  std::uniform_int_distribution<std::size_t> qubit(0, n - 1);
  std::uniform_real_distribution<double> angle(0.0, 6.283185307179586);
  std::vector<Op> ops;
  for (int i = 0; i < count; ++i) {
    const std::size_t a = qubit(rng);
    std::size_t b = qubit(rng);
    while (b == a) b = qubit(rng);
    ops.push_back({kind(rng), a, b, angle(rng)});
  }
  return ops;
}

void apply_ops(ket::Circuit& c, const std::vector<Op>& ops, bool barriers) {
  for (const Op& op : ops) {
    switch (op.kind) {
      case 0:
        c.h(op.a);
        break;
      case 1:
        c.ry(op.a, op.theta);
        break;
      case 2:
        c.rz(op.a, op.theta);
        break;
      case 3:
        c.cx(op.a, op.b);
        break;
      case 4:
        c.cz(op.a, op.b);
        break;
    }
    if (barriers) c.barrier();
  }
}

}  // namespace

TEST(Fusion, MatchesUnfused) {
  ket::set_num_threads(4);  // also exercises the parallel fused path
  std::mt19937 rng{99};
  const std::size_t n = 20;  // 2^20 amplitudes — the fused path is active
  const std::vector<Op> ops = random_ops(n, 60, rng);

  ket::Circuit fused{n};
  ket::Circuit unfused{n};
  apply_ops(fused, ops, false);   // runs as fused blocks
  apply_ops(unfused, ops, true);  // a barrier after each gate -> one at a time

  const ket::State a = ket::run(fused);
  const ket::State b = ket::run(unfused);
  ket::set_num_threads(1);
  ASSERT_EQ(a.size(), b.size());
  for (std::size_t i = 0; i < a.size(); ++i) {
    EXPECT_NEAR(a[i].real(), b[i].real(), 1e-9) << "i=" << i;
    EXPECT_NEAR(a[i].imag(), b[i].imag(), 1e-9) << "i=" << i;
  }
}

// A 20-qubit GHZ exercises fused blocks that contain entangling gates; its
// state is analytic: only |0...0> and |1...1> are populated, each at 1/sqrt(2).
TEST(Fusion, GhzAmplitudes) {
  const std::size_t n = 20;
  ket::Circuit c{n};
  c.h(0);
  for (std::size_t i = 0; i + 1 < n; ++i) c.cx(i, i + 1);

  const ket::State s = ket::run(c);
  const double inv_sqrt2 = 1.0 / std::sqrt(2.0);
  EXPECT_NEAR(s[0].real(), inv_sqrt2, 1e-12);
  EXPECT_NEAR(s[s.size() - 1].real(), inv_sqrt2, 1e-12);
  EXPECT_NEAR(std::abs(s[1]), 0.0, 1e-12);
  EXPECT_NEAR(std::abs(s[s.size() / 2]), 0.0, 1e-12);
}
