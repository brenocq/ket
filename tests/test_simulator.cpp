#include <gtest/gtest.h>
#include <ket/circuit.hpp>
#include <ket/simulator.hpp>

#include <cmath>

namespace {

constexpr double kTol = 1e-12;

void ExpectAmplitude(ket::Complex got, ket::Complex want) {
  EXPECT_NEAR(got.real(), want.real(), kTol);
  EXPECT_NEAR(got.imag(), want.imag(), kTol);
}

}  // namespace

TEST(Simulator, EmptyCircuitReturnsGroundState) {
  ket::Circuit c{2};
  auto s = ket::run(c);
  ASSERT_EQ(s.size(), 4u);
  ExpectAmplitude(s[0], {1.0, 0.0});
  ExpectAmplitude(s[1], {0.0, 0.0});
  ExpectAmplitude(s[2], {0.0, 0.0});
  ExpectAmplitude(s[3], {0.0, 0.0});
}

TEST(Simulator, XFlipsZeroToOne) {
  ket::Circuit c{1};
  c.x(0);
  auto s = ket::run(c);
  ExpectAmplitude(s[0], {0.0, 0.0});
  ExpectAmplitude(s[1], {1.0, 0.0});
}

TEST(Simulator, HCreatesSuperposition) {
  ket::Circuit c{1};
  c.h(0);
  auto s = ket::run(c);
  const double inv_sqrt2 = 1.0 / std::sqrt(2.0);
  ExpectAmplitude(s[0], {inv_sqrt2, 0.0});
  ExpectAmplitude(s[1], {inv_sqrt2, 0.0});
}

TEST(Simulator, HHIsIdentity) {
  ket::Circuit c{1};
  c.h(0);
  c.h(0);
  auto s = ket::run(c);
  ExpectAmplitude(s[0], {1.0, 0.0});
  ExpectAmplitude(s[1], {0.0, 0.0});
}

TEST(Simulator, ZIntroducesMinusSignOnOne) {
  ket::Circuit c{1};
  c.x(0);
  c.z(0);
  auto s = ket::run(c);
  ExpectAmplitude(s[0], {0.0, 0.0});
  ExpectAmplitude(s[1], {-1.0, 0.0});
}

TEST(Simulator, CNOTFlipsTargetWhenControlIsOne) {
  ket::Circuit c{2};
  c.x(0);
  c.cnot(0, 1);
  auto s = ket::run(c);
  ExpectAmplitude(s[0], {0.0, 0.0});
  ExpectAmplitude(s[1], {0.0, 0.0});
  ExpectAmplitude(s[2], {0.0, 0.0});
  ExpectAmplitude(s[3], {1.0, 0.0});
}

TEST(Simulator, CNOTDoesNothingWhenControlIsZero) {
  ket::Circuit c{2};
  c.cnot(0, 1);
  auto s = ket::run(c);
  ExpectAmplitude(s[0], {1.0, 0.0});
  ExpectAmplitude(s[1], {0.0, 0.0});
  ExpectAmplitude(s[2], {0.0, 0.0});
  ExpectAmplitude(s[3], {0.0, 0.0});
}

TEST(Simulator, PrintRendersBasisStatesAndAmplitudes) {
  ket::Circuit c{2};
  c.h(0);
  c.cnot(0, 1);
  auto s = ket::run(c);
  EXPECT_EQ(s.print(),
            "|00⟩: 0.707107\n"
            "|01⟩: 0\n"
            "|10⟩: 0\n"
            "|11⟩: 0.707107\n");
}

TEST(Simulator, PrintShowsNegativeRealAmplitude) {
  ket::Circuit c{1};
  c.x(0);
  c.z(0);
  auto s = ket::run(c);
  EXPECT_EQ(s.print(),
            "|0⟩: 0\n"
            "|1⟩: -1\n");
}

TEST(Simulator, BellCircuitProducesEntangledState) {
  ket::Circuit c{2};
  c.h(0);
  c.cnot(0, 1);
  auto s = ket::run(c);
  const double inv_sqrt2 = 1.0 / std::sqrt(2.0);
  ExpectAmplitude(s[0], {inv_sqrt2, 0.0});
  ExpectAmplitude(s[1], {0.0, 0.0});
  ExpectAmplitude(s[2], {0.0, 0.0});
  ExpectAmplitude(s[3], {inv_sqrt2, 0.0});
}

TEST(Simulator, RyHalfPiMakesEqualRealSuperposition) {
  const double pi = std::acos(-1.0);
  ket::Circuit c{1};
  c.ry(0, pi / 2.0);
  auto s = ket::run(c);
  const double inv_sqrt2 = 1.0 / std::sqrt(2.0);
  ExpectAmplitude(s[0], {inv_sqrt2, 0.0});
  ExpectAmplitude(s[1], {inv_sqrt2, 0.0});
}

TEST(Simulator, RxPiFlipsZeroToMinusIOne) {
  const double pi = std::acos(-1.0);
  ket::Circuit c{1};
  c.rx(0, pi);
  auto s = ket::run(c);
  ExpectAmplitude(s[0], {0.0, 0.0});
  ExpectAmplitude(s[1], {0.0, -1.0});  // Rx(pi)|0> = -i|1>
}

TEST(Simulator, RzPreservesComputationalProbabilities) {
  const double pi = std::acos(-1.0);
  ket::Circuit c{1};
  c.h(0);
  c.rz(0, pi / 2.0);  // only changes the relative phase
  auto s = ket::run(c);
  EXPECT_NEAR(std::norm(s[0]), 0.5, 1e-12);
  EXPECT_NEAR(std::norm(s[1]), 0.5, 1e-12);
}

TEST(Simulator, BarrierIsNoOp) {
  ket::Circuit c{2};
  c.h(0);
  c.barrier();
  c.cnot(0, 1);
  c.barrier({1}, "done");
  auto s = ket::run(c);
  const double inv_sqrt2 = 1.0 / std::sqrt(2.0);
  ExpectAmplitude(s[0], {inv_sqrt2, 0.0});
  ExpectAmplitude(s[1], {0.0, 0.0});
  ExpectAmplitude(s[2], {0.0, 0.0});
  ExpectAmplitude(s[3], {inv_sqrt2, 0.0});
}
