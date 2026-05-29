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
