// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
//
// The stabilizer backend must agree with the dense state-vector backend on
// every Clifford circuit. These tests pin that down exhaustively (all Pauli
// strings, random Clifford circuits) plus the dispatch/selection behavior.
#include <gtest/gtest.h>
#include <ket/backends/backend.hpp>
#include <ket/circuit.hpp>
#include <ket/measurement.hpp>
#include <ket/observable.hpp>
#include <ket/simulator.hpp>

#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

constexpr double kTol = 1e-9;

// A random Clifford circuit on n qubits with `depth` gates, drawn from the
// gate set the stabilizer backend supports.
ket::Circuit random_clifford(std::size_t n, int depth, std::mt19937& rng) {
  ket::Circuit c{n};
  std::uniform_int_distribution<int> gate(0, 9);
  std::uniform_int_distribution<std::size_t> qubit(0, n - 1);
  for (int d = 0; d < depth; ++d) {
    const std::size_t a = qubit(rng);
    std::size_t b = qubit(rng);
    while (n > 1 && b == a) b = qubit(rng);
    switch (gate(rng)) {
      case 0:
        c.h(a);
        break;
      case 1:
        c.s(a);
        break;
      case 2:
        c.sdg(a);
        break;
      case 3:
        c.x(a);
        break;
      case 4:
        c.y(a);
        break;
      case 5:
        c.z(a);
        break;
      case 6:
        if (n > 1) c.cx(a, b);
        break;
      case 7:
        if (n > 1) c.cy(a, b);
        break;
      case 8:
        if (n > 1) c.cz(a, b);
        break;
      case 9:
        if (n > 1) c.swap(a, b);
        break;
    }
  }
  return c;
}

// Every length-n Pauli string ("II", "IX", ..., "ZZ").
std::vector<std::string> all_paulis(std::size_t n) {
  const char letters[] = {'I', 'X', 'Y', 'Z'};
  std::vector<std::string> out;
  std::size_t total = 1;
  for (std::size_t i = 0; i < n; ++i) total *= 4;
  for (std::size_t k = 0; k < total; ++k) {
    std::string p(n, 'I');
    std::size_t v = k;
    for (std::size_t i = 0; i < n; ++i) {
      p[n - 1 - i] = letters[v % 4];
      v /= 4;
    }
    out.push_back(p);
  }
  return out;
}

}  // namespace

// The decisive test: stabilizer expectation values must equal the dense ones
// for every Pauli observable on random Clifford circuits.
TEST(Stabilizer, ExpvalMatchesStateVector) {
  std::mt19937 rng{1234};
  for (std::size_t n = 1; n <= 4; ++n) {
    for (int trial = 0; trial < 20; ++trial) {
      const ket::Circuit c = random_clifford(n, 25, rng);
      const ket::State dense = ket::run(c);
      for (const std::string& p : all_paulis(n)) {
        const double want = ket::expval(dense, p);
        EXPECT_NEAR(ket::expval(c, p, ket::Method::Stabilizer), want, kTol)
            << "n=" << n << " trial=" << trial << " pauli=" << p;
        // Auto routes Clifford circuits to the stabilizer backend.
        EXPECT_NEAR(ket::expval(c, p), want, kTol);
      }
    }
  }
}

TEST(Stabilizer, IsCliffordDetection) {
  ket::Circuit clifford{2};
  clifford.h(0);
  clifford.s(1);
  clifford.cx(0, 1);
  clifford.cz(0, 1);
  EXPECT_TRUE(ket::is_clifford(clifford));
  EXPECT_EQ(ket::chosen_method(clifford), ket::Method::Stabilizer);

  ket::Circuit with_t{1};
  with_t.t(0);  // T is the canonical non-Clifford gate
  EXPECT_FALSE(ket::is_clifford(with_t));
  EXPECT_EQ(ket::chosen_method(with_t), ket::Method::StateVector);

  ket::Circuit with_rotation{1};
  with_rotation.rx(0, 0.3);
  EXPECT_FALSE(ket::is_clifford(with_rotation));

  ket::Circuit toffoli{3};
  toffoli.ccx(0, 1, 2);
  EXPECT_FALSE(ket::is_clifford(toffoli));
}

TEST(Stabilizer, CliffordCompositeIsClifford) {
  ket::Circuit bell{2};
  bell.h(0);
  bell.cx(0, 1);
  ket::Circuit outer{2};
  outer.append(bell, {0, 1});
  EXPECT_TRUE(ket::is_clifford(outer));

  ket::Circuit non_clifford_block{1};
  non_clifford_block.t(0);
  ket::Circuit outer2{1};
  outer2.append(non_clifford_block, {0});
  EXPECT_FALSE(ket::is_clifford(outer2));
}

TEST(Stabilizer, ExplicitStabilizerOnNonCliffordThrows) {
  ket::Circuit c{1};
  c.t(0);
  EXPECT_THROW(ket::simulate(c, ket::Method::Stabilizer),
               std::invalid_argument);
  EXPECT_THROW(ket::expval(c, "Z", ket::Method::Stabilizer),
               std::invalid_argument);
  // Auto still works (falls back to the dense backend).
  EXPECT_NO_THROW(ket::expval(c, "Z"));
}

TEST(Stabilizer, BackendNames) {
  ket::Circuit clifford{1};
  clifford.h(0);
  EXPECT_EQ(ket::simulate(clifford)->backend(), "stabilizer");

  ket::Circuit dense{1};
  dense.t(0);
  EXPECT_EQ(ket::simulate(dense)->backend(), "state vector");
}

// GHZ: every measured qubit must agree, and both 0...0 and 1...1 appear.
TEST(Stabilizer, GhzSamplingCorrelations) {
  ket::Circuit c{3};
  c.h(0);
  c.cx(0, 1);
  c.cx(1, 2);
  c.measure_all();

  std::mt19937 rng{7};
  const auto sim = ket::simulate(c, ket::Method::Stabilizer);
  ASSERT_EQ(sim->backend(), "stabilizer");
  int zeros = 0, ones = 0;
  for (int shot = 0; shot < 400; ++shot) {
    const std::vector<int> creg = sim->sample(rng);
    ASSERT_EQ(creg.size(), 3u);
    EXPECT_EQ(creg[0], creg[1]);
    EXPECT_EQ(creg[1], creg[2]);
    (creg[0] == 0 ? zeros : ones)++;
  }
  EXPECT_GT(zeros, 0);
  EXPECT_GT(ones, 0);
}

// A deterministic Clifford preparation must sample its single outcome every
// time, matching what the dense backend would give.
TEST(Stabilizer, DeterministicOutcome) {
  ket::Circuit c{2};
  c.x(0);  // |1>
  c.h(1);
  c.h(1);  // back to |0>
  c.measure_all();

  std::mt19937 rng{0};
  const auto sim = ket::simulate(c, ket::Method::Stabilizer);
  for (int shot = 0; shot < 50; ++shot) {
    const std::vector<int> creg = sim->sample(rng);
    EXPECT_EQ(creg[0], 1);
    EXPECT_EQ(creg[1], 0);
  }
}