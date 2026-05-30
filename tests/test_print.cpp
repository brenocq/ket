#include <gtest/gtest.h>
#include <ket/circuit.hpp>

#include <cmath>
#include <string>

TEST(Print, EmptyOneQubit) {
  ket::Circuit c{1};
  EXPECT_EQ(c.print(),
            "          \n"
            "q_0: ─────\n"
            "          \n");
}

TEST(Print, SingleHadamard) {
  ket::Circuit c{1};
  c.h(0);
  EXPECT_EQ(c.print(),
            "     ┌───┐\n"
            "q_0: ┤ H ├\n"
            "     └───┘\n");
}

TEST(Print, SingleQubitSequence) {
  ket::Circuit c{1};
  c.h(0);
  c.x(0);
  c.z(0);
  EXPECT_EQ(c.print(),
            "     ┌───┐┌───┐┌───┐\n"
            "q_0: ┤ H ├┤ X ├┤ Z ├\n"
            "     └───┘└───┘└───┘\n");
}

TEST(Print, BellCircuit) {
  ket::Circuit c{2};
  c.h(0);
  c.cx(0, 1);
  EXPECT_EQ(c.print(),
            "     ┌───┐     \n"
            "q_0: ┤ H ├──■──\n"
            "     └───┘┌─┴─┐\n"
            "q_1: ─────┤ X ├\n"
            "          └───┘\n");
}

TEST(Print, CnotControlBelowTarget) {
  ket::Circuit c{2};
  c.cx(1, 0);
  EXPECT_EQ(c.print(),
            "     ┌───┐\n"
            "q_0: ┤ X ├\n"
            "     └─┬─┘\n"
            "q_1: ──■──\n"
            "          \n");
}

TEST(Print, CnotSpansIntermediateQubit) {
  ket::Circuit c{3};
  c.cx(0, 2);
  EXPECT_EQ(c.print(),
            "          \n"
            "q_0: ──■──\n"
            "       │  \n"
            "q_1: ──┼──\n"
            "     ┌─┴─┐\n"
            "q_2: ┤ X ├\n"
            "     └───┘\n");
}

TEST(Print, BarrierFullSpan) {
  ket::Circuit c{2};
  c.h(0);
  c.barrier();
  EXPECT_EQ(c.print(),
            "     ┌───┐░\n"
            "q_0: ┤ H ├░\n"
            "     └───┘░\n"
            "q_1: ─────░\n"
            "          ░\n");
}

TEST(Print, BarrierWithLabel) {
  ket::Circuit c{2};
  c.h(0);
  c.barrier("m");
  EXPECT_EQ(c.print(),
            "          m\n"
            "     ┌───┐░\n"
            "q_0: ┤ H ├░\n"
            "     └───┘░\n"
            "q_1: ─────░\n"
            "          ░\n");
}

TEST(Print, BarrierSubsetLeavesOtherWiresConnected) {
  ket::Circuit c{3};
  c.barrier({1, 2});
  EXPECT_EQ(c.print(),
            "      \n"
            "q_0: ─\n"
            "     ░\n"
            "q_1: ░\n"
            "     ░\n"
            "q_2: ░\n"
            "     ░\n");
}

TEST(Print, MeasureSingleQubit) {
  ket::Circuit c{1};
  c.measure(0, 0);
  EXPECT_EQ(c.print(),
            "     ┌───┐\n"
            "q_0: ┤ M ├\n"
            "     └─╥─┘\n"
            "c: 1/══╩══\n"
            "       0  \n");
}

TEST(Print, MeasureBellIntoClassicalRegister) {
  ket::Circuit c{2};
  c.h(0);
  c.cx(0, 1);
  c.measure_all();
  EXPECT_EQ(c.print(),
            "     ┌───┐     ┌───┐     \n"
            "q_0: ┤ H ├──■──┤ M ├─────\n"
            "     └───┘┌─┴─┐└─╥─┘┌───┐\n"
            "q_1: ─────┤ X ├──╫──┤ M ├\n"
            "          └───┘  ║  └─╥─┘\n"
            "c: 2/════════════╩════╩══\n"
            "                 0    1  \n");
}

TEST(Print, RotationGateBox) {
  ket::Circuit c{1};
  c.ry(0, 1.5);
  EXPECT_EQ(c.print(),
            "     ┌─────────┐\n"
            "q_0: ┤ Ry(1.5) ├\n"
            "     └─────────┘\n");
}

TEST(Print, RotationPiExactBox) {
  const double pi = std::acos(-1.0);
  ket::Circuit c{1};
  c.rx(0, pi);
  EXPECT_EQ(c.print(),
            "     ┌───────┐\n"
            "q_0: ┤ Rx(π) ├\n"
            "     └───────┘\n");
}

TEST(Print, RotationAnglesUsePiSymbols) {
  const double pi = std::acos(-1.0);
  auto label_of = [](const ket::Circuit& c) { return c.print(); };

  {
    ket::Circuit c{1};
    c.ry(0, pi / 2.0);
    EXPECT_NE(label_of(c).find("Ry(π/2)"), std::string::npos);
  }
  {
    ket::Circuit c{1};
    c.rz(0, pi / 4.0);
    EXPECT_NE(label_of(c).find("Rz(π/4)"), std::string::npos);
  }
  {
    ket::Circuit c{1};
    c.rx(0, 3.0 * pi / 4.0);
    EXPECT_NE(label_of(c).find("Rx(3π/4)"), std::string::npos);
  }
  {
    ket::Circuit c{1};
    c.rz(0, 2.0 * pi);
    EXPECT_NE(label_of(c).find("Rz(2π)"), std::string::npos);
  }
  {
    ket::Circuit c{1};
    c.rz(0, -pi / 2.0);
    EXPECT_NE(label_of(c).find("Rz(-π/2)"), std::string::npos);
  }
  {
    // A non-π angle keeps the decimal form.
    ket::Circuit c{1};
    c.rx(0, 1.5);
    EXPECT_NE(label_of(c).find("Rx(1.5)"), std::string::npos);
  }
}

TEST(Print, ControlledZSymmetricDots) {
  ket::Circuit c{2};
  c.cz(0, 1);
  EXPECT_EQ(c.print(),
            "          \n"
            "q_0: ──■──\n"
            "       │  \n"
            "q_1: ──■──\n"
            "          \n");
}

TEST(Print, ControlledPhaseBox) {
  const double pi = std::acos(-1.0);
  ket::Circuit c{2};
  c.cp(0, 1, pi / 2.0);
  EXPECT_EQ(c.print(),
            "               \n"
            "q_0: ─────■────\n"
            "     ┌────┴───┐\n"
            "q_1: ┤ P(π/2) ├\n"
            "     └────────┘\n");
}

TEST(Print, ProbeMarkersAtBottom) {
  ket::Circuit c{2};
  c.h(0);
  c.probe();  // auto -> ψ0
  c.cx(0, 1);
  c.probe();  // auto -> ψ1
  EXPECT_EQ(c.print(),
            "     ┌───┐       \n"
            "q_0: ┤ H ├───■───\n"
            "     └───┘ ┌─┴─┐ \n"
            "q_1: ──────┤ X ├─\n"
            "           └───┘ \n"
            "          ↑     ↑\n"
            "          ψ0    ψ1\n");
}

TEST(Print, UGateBox) {
  const double pi = std::acos(-1.0);
  ket::Circuit c{1};
  c.u(0, pi / 2.0, 0.0, pi);
  EXPECT_NE(c.print().find("U(π/2,0,π)"), std::string::npos);
}

TEST(Print, NoClassicalRowsWithoutMeasurement) {
  // A circuit without measurements renders exactly as before (no bus row).
  ket::Circuit c{1};
  c.h(0);
  EXPECT_EQ(c.print(),
            "     ┌───┐\n"
            "q_0: ┤ H ├\n"
            "     └───┘\n");
}
