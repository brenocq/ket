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
  c.cx(0, 1);
  auto s = ket::run(c);
  ExpectAmplitude(s[0], {0.0, 0.0});
  ExpectAmplitude(s[1], {0.0, 0.0});
  ExpectAmplitude(s[2], {0.0, 0.0});
  ExpectAmplitude(s[3], {1.0, 0.0});
}

TEST(Simulator, CNOTDoesNothingWhenControlIsZero) {
  ket::Circuit c{2};
  c.cx(0, 1);
  auto s = ket::run(c);
  ExpectAmplitude(s[0], {1.0, 0.0});
  ExpectAmplitude(s[1], {0.0, 0.0});
  ExpectAmplitude(s[2], {0.0, 0.0});
  ExpectAmplitude(s[3], {0.0, 0.0});
}

TEST(Simulator, PrintRendersBasisStatesAndAmplitudes) {
  ket::Circuit c{2};
  c.h(0);
  c.cx(0, 1);
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
  c.cx(0, 1);
  auto s = ket::run(c);
  const double inv_sqrt2 = 1.0 / std::sqrt(2.0);
  ExpectAmplitude(s[0], {inv_sqrt2, 0.0});
  ExpectAmplitude(s[1], {0.0, 0.0});
  ExpectAmplitude(s[2], {0.0, 0.0});
  ExpectAmplitude(s[3], {inv_sqrt2, 0.0});
}

TEST(Simulator, SAndTPhaseTheOneState) {
  const double inv_sqrt2 = 1.0 / std::sqrt(2.0);
  {  // S|1> = i|1>
    ket::Circuit c{1};
    c.x(0);
    c.s(0);
    ExpectAmplitude(ket::run(c)[1], {0.0, 1.0});
  }
  {  // Sdg|1> = -i|1>
    ket::Circuit c{1};
    c.x(0);
    c.sdg(0);
    ExpectAmplitude(ket::run(c)[1], {0.0, -1.0});
  }
  {  // T|1> = e^{i pi/4}|1>
    ket::Circuit c{1};
    c.x(0);
    c.t(0);
    ExpectAmplitude(ket::run(c)[1], {inv_sqrt2, inv_sqrt2});
  }
  {  // Tdg|1> = e^{-i pi/4}|1>
    ket::Circuit c{1};
    c.x(0);
    c.tdg(0);
    ExpectAmplitude(ket::run(c)[1], {inv_sqrt2, -inv_sqrt2});
  }
}

TEST(Simulator, TTwiceEqualsS) {
  // T^2 = S, so T then T on |1> gives i|1>.
  ket::Circuit c{1};
  c.x(0);
  c.t(0);
  c.t(0);
  ExpectAmplitude(ket::run(c)[1], {0.0, 1.0});
}

TEST(Simulator, YFlipsZeroToIOne) {
  ket::Circuit c{1};
  c.y(0);
  auto s = ket::run(c);
  ExpectAmplitude(s[0], {0.0, 0.0});
  ExpectAmplitude(s[1], {0.0, 1.0});  // Y|0> = i|1>
}

TEST(Simulator, CzPhasesOnlyTheOneOneState) {
  ket::Circuit c{2};
  c.x(0);
  c.x(1);
  c.cz(0, 1);  // -1 on |11>
  auto s = ket::run(c);
  ExpectAmplitude(s[3], {-1.0, 0.0});
}

TEST(Simulator, CzLeavesOtherStatesUnchanged) {
  ket::Circuit c{2};
  c.x(0);  // |01>
  c.cz(0, 1);
  auto s = ket::run(c);
  ExpectAmplitude(s[1], {1.0, 0.0});  // only one qubit set -> no phase
}

TEST(Simulator, CpAppliesPhaseOnOneOne) {
  const double pi = std::acos(-1.0);
  ket::Circuit c{2};
  c.x(0);
  c.x(1);
  c.cp(0, 1, pi / 2.0);  // e^{i pi/2} = i on |11>
  auto s = ket::run(c);
  ExpectAmplitude(s[3], {0.0, 1.0});
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

TEST(Simulator, UGateRecoversKnownGates) {
  const double pi = std::acos(-1.0);
  const double inv_sqrt2 = 1.0 / std::sqrt(2.0);
  {  // U(pi/2, 0, pi) == H
    ket::Circuit c{1};
    c.u(0, pi / 2.0, 0.0, pi);
    auto s = ket::run(c);
    ExpectAmplitude(s[0], {inv_sqrt2, 0.0});
    ExpectAmplitude(s[1], {inv_sqrt2, 0.0});
  }
  {  // U(pi, 0, pi) == X
    ket::Circuit c{1};
    c.u(0, pi, 0.0, pi);
    auto s = ket::run(c);
    ExpectAmplitude(s[0], {0.0, 0.0});
    ExpectAmplitude(s[1], {1.0, 0.0});
  }
  {  // U(0, 0, lambda) == phase: on |1> gives e^{i lambda}|1>
    ket::Circuit c{1};
    c.x(0);
    c.u(0, 0.0, 0.0, pi / 2.0);
    ExpectAmplitude(ket::run(c)[1], {0.0, 1.0});  // e^{i pi/2} = i
  }
}

TEST(Simulator, RunWithProbesCapturesStages) {
  ket::Circuit c{2};
  c.h(0);
  c.probe("a");
  c.cx(0, 1);
  c.probe("b");

  ket::ProbeRun r = ket::run_with_probes(c);
  ASSERT_EQ(r.probes.size(), 2u);
  EXPECT_EQ(r.probes[0].first, "a");
  EXPECT_EQ(r.probes[1].first, "b");

  const double inv_sqrt2 = 1.0 / std::sqrt(2.0);
  // ψ_a, after H on q0: (|00> + |01>)/sqrt(2)
  ExpectAmplitude(r.probes[0].second[0], {inv_sqrt2, 0.0});
  ExpectAmplitude(r.probes[0].second[1], {inv_sqrt2, 0.0});
  ExpectAmplitude(r.probes[0].second[3], {0.0, 0.0});
  // ψ_b, after CNOT: the Bell state
  ExpectAmplitude(r.probes[1].second[0], {inv_sqrt2, 0.0});
  ExpectAmplitude(r.probes[1].second[3], {inv_sqrt2, 0.0});
  // final state matches the last stage
  ExpectAmplitude(r.final[3], {inv_sqrt2, 0.0});
}

TEST(Simulator, ProbeIsNoOpForRun) {
  ket::Circuit c{1};
  c.x(0);
  c.probe();
  ExpectAmplitude(ket::run(c)[1], {1.0, 0.0});
}

TEST(Simulator, BarrierIsNoOp) {
  ket::Circuit c{2};
  c.h(0);
  c.barrier();
  c.cx(0, 1);
  c.barrier({1}, "done");
  auto s = ket::run(c);
  const double inv_sqrt2 = 1.0 / std::sqrt(2.0);
  ExpectAmplitude(s[0], {inv_sqrt2, 0.0});
  ExpectAmplitude(s[1], {0.0, 0.0});
  ExpectAmplitude(s[2], {0.0, 0.0});
  ExpectAmplitude(s[3], {inv_sqrt2, 0.0});
}
