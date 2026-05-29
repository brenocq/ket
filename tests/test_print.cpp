#include <gtest/gtest.h>
#include <ket/circuit.hpp>

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
  c.cnot(0, 1);
  EXPECT_EQ(c.print(),
            "     ┌───┐     \n"
            "q_0: ┤ H ├──■──\n"
            "     └───┘┌─┴─┐\n"
            "q_1: ─────┤ X ├\n"
            "          └───┘\n");
}

TEST(Print, CnotControlBelowTarget) {
  ket::Circuit c{2};
  c.cnot(1, 0);
  EXPECT_EQ(c.print(),
            "     ┌───┐\n"
            "q_0: ┤ X ├\n"
            "     └─┬─┘\n"
            "q_1: ──■──\n"
            "          \n");
}

TEST(Print, CnotSpansIntermediateQubit) {
  ket::Circuit c{3};
  c.cnot(0, 2);
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
