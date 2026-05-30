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

TEST(Simulator, ControlledRotationsActOnlyWhenControlIsOne) {
  const double pi = std::acos(-1.0);
  {  // control=0: crx is a no-op
    ket::Circuit c{2};
    c.crx(0, 1, pi);
    ExpectAmplitude(ket::run(c)[0], {1.0, 0.0});
  }
  {  // control=1: crx(pi) acts like rx(pi) on the target -> -i|11>
    ket::Circuit c{2};
    c.x(0);  // |01>
    c.crx(0, 1, pi);
    ExpectAmplitude(ket::run(c)[3], {0.0, -1.0});
  }
  {  // control=1: cry(pi/2) makes an equal real superposition on the target
    const double inv_sqrt2 = 1.0 / std::sqrt(2.0);
    ket::Circuit c{2};
    c.x(0);  // |01>
    c.cry(0, 1, pi / 2.0);
    auto s = ket::run(c);
    ExpectAmplitude(s[1], {inv_sqrt2, 0.0});  // |01>
    ExpectAmplitude(s[3], {inv_sqrt2, 0.0});  // |11>
  }
  {  // crz only adds a relative phase; probabilities are unchanged
    ket::Circuit c{2};
    c.x(0);  // control=1
    c.h(1);  // target in superposition
    c.crz(0, 1, pi / 2.0);
    auto s = ket::run(c);
    EXPECT_NEAR(std::norm(s[1]), 0.5, 1e-12);  // |01>
    EXPECT_NEAR(std::norm(s[3]), 0.5, 1e-12);  // |11>
  }
}

TEST(Simulator, CcxFlipsTargetOnlyWhenBothControlsAreOne) {
  {  // only one control set -> no flip
    ket::Circuit c{3};
    c.x(0);  // |001>
    c.ccx(0, 1, 2);
    ExpectAmplitude(ket::run(c)[1], {1.0, 0.0});  // still |001>
  }
  {  // both controls set -> target flips: |011> -> |111>
    ket::Circuit c{3};
    c.x(0);
    c.x(1);  // |011>
    c.ccx(0, 1, 2);
    auto s = ket::run(c);
    ExpectAmplitude(s[3], {0.0, 0.0});  // |011> emptied
    ExpectAmplitude(s[7], {1.0, 0.0});  // |111>
  }
}

TEST(Simulator, CswapExchangesTargetsOnlyWhenControlIsOne) {
  {  // control=0: no swap. |010> stays put.
    ket::Circuit c{3};
    c.x(1);  // |010> (index 2)
    c.cswap(0, 1, 2);
    ExpectAmplitude(ket::run(c)[2], {1.0, 0.0});
  }
  {  // control=1: swap targets. |011> -> |101>
    ket::Circuit c{3};
    c.x(0);  // control
    c.x(1);  // |011> (index 3)
    c.cswap(0, 1, 2);
    auto s = ket::run(c);
    ExpectAmplitude(s[3], {0.0, 0.0});  // |011> emptied
    ExpectAmplitude(s[5], {1.0, 0.0});  // |101>
  }
}

TEST(Simulator, CuAppliesUnitaryWhenControlIsOne) {
  const double pi = std::acos(-1.0);
  const double inv_sqrt2 = 1.0 / std::sqrt(2.0);
  {  // control=0: no-op
    ket::Circuit c{2};
    c.cu(0, 1, pi / 2.0, 0.0, pi);
    ExpectAmplitude(ket::run(c)[0], {1.0, 0.0});
  }
  {  // control=1: U(pi/2,0,pi) == H on the target. |01> -> (|01>+|11>)/sqrt2
    ket::Circuit c{2};
    c.x(0);
    c.cu(0, 1, pi / 2.0, 0.0, pi);
    auto s = ket::run(c);
    ExpectAmplitude(s[1], {inv_sqrt2, 0.0});  // |01>
    ExpectAmplitude(s[3], {inv_sqrt2, 0.0});  // |11>
  }
}

TEST(Simulator, ChAppliesHWhenControlIsOne) {
  const double inv_sqrt2 = 1.0 / std::sqrt(2.0);
  {  // control=0: no-op
    ket::Circuit c{2};
    c.ch(0, 1);
    ExpectAmplitude(ket::run(c)[0], {1.0, 0.0});
  }
  {  // control=1: H on target. |01> (q0=1) -> (|01> + |11>)/sqrt(2)
    ket::Circuit c{2};
    c.x(0);
    c.ch(0, 1);
    auto s = ket::run(c);
    ExpectAmplitude(s[1], {inv_sqrt2, 0.0});  // |01>
    ExpectAmplitude(s[3], {inv_sqrt2, 0.0});  // |11>
  }
}

TEST(Simulator, CyAppliesYWhenControlIsOne) {
  {  // control=0: nothing happens
    ket::Circuit c{2};
    c.cy(0, 1);
    ExpectAmplitude(ket::run(c)[0], {1.0, 0.0});
  }
  {  // control=1: Y on the target. |01> (q0=1) -> i|11>
    ket::Circuit c{2};
    c.x(0);
    c.cy(0, 1);
    auto s = ket::run(c);
    ExpectAmplitude(s[1], {0.0, 0.0});
    ExpectAmplitude(s[3], {0.0, 1.0});  // i|11>
  }
}

TEST(Simulator, SwapExchangesQubits) {
  ket::Circuit c{2};
  c.x(0);        // |01> (qubit 0 set)
  c.swap(0, 1);  // -> |10> (qubit 1 set)
  auto s = ket::run(c);
  ExpectAmplitude(s[1], {0.0, 0.0});  // |01> emptied
  ExpectAmplitude(s[2], {1.0, 0.0});  // |10> populated
}

TEST(Simulator, SwapIsThreeCnots) {
  // swap == cx(a,b); cx(b,a); cx(a,b); on an arbitrary state.
  ket::Circuit a{2};
  a.h(0);
  a.swap(0, 1);

  ket::Circuit b{2};
  b.h(0);
  b.cx(0, 1);
  b.cx(1, 0);
  b.cx(0, 1);

  auto sa = ket::run(a);
  auto sb = ket::run(b);
  for (std::size_t i = 0; i < sa.size(); ++i) {
    ExpectAmplitude(sa[i], sb[i]);
  }
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
