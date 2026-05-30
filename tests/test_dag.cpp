#include <gtest/gtest.h>
#include <ket/circuit.hpp>
#include <ket/dag.hpp>

TEST(Dag, EmptyCircuitHasNoNodes) {
  ket::Circuit c{2};
  EXPECT_EQ(c.dag().nodes().size(), 0u);
  EXPECT_EQ(c.dag().n_qubits(), 2u);
}

TEST(Dag, BellCircuitStructure) {
  ket::Circuit c{2};
  c.h(0);
  c.cx(0, 1);

  const auto& nodes = c.dag().nodes();
  ASSERT_EQ(nodes.size(), 2u);

  EXPECT_EQ(nodes[0].gate.type, ket::GateType::H);
  EXPECT_TRUE(nodes[0].predecessors.empty());
  ASSERT_EQ(nodes[0].successors.size(), 1u);
  EXPECT_EQ(nodes[0].successors[0], 1u);

  EXPECT_EQ(nodes[1].gate.type, ket::GateType::CX);
  ASSERT_EQ(nodes[1].predecessors.size(), 1u);
  EXPECT_EQ(nodes[1].predecessors[0], 0u);
  EXPECT_TRUE(nodes[1].successors.empty());
}

TEST(Dag, IndependentGatesAreNotConnected) {
  ket::Circuit c{2};
  c.h(0);
  c.x(1);

  const auto& nodes = c.dag().nodes();
  ASSERT_EQ(nodes.size(), 2u);
  EXPECT_TRUE(nodes[0].successors.empty());
  EXPECT_TRUE(nodes[0].predecessors.empty());
  EXPECT_TRUE(nodes[1].successors.empty());
  EXPECT_TRUE(nodes[1].predecessors.empty());
}

TEST(Dag, ConsecutiveTwoQubitGatesDedupePredecessor) {
  ket::Circuit c{2};
  c.cx(0, 1);
  c.cx(0, 1);

  const auto& nodes = c.dag().nodes();
  ASSERT_EQ(nodes.size(), 2u);
  ASSERT_EQ(nodes[1].predecessors.size(), 1u);
  EXPECT_EQ(nodes[1].predecessors[0], 0u);
  ASSERT_EQ(nodes[0].successors.size(), 1u);
  EXPECT_EQ(nodes[0].successors[0], 1u);
}

TEST(Dag, WiresTrackNodesPerQubit) {
  ket::Circuit c{2};
  c.h(0);
  c.x(1);
  c.cx(0, 1);

  EXPECT_EQ(c.dag().wire(0).size(), 2u);
  EXPECT_EQ(c.dag().wire(0)[0], 0u);
  EXPECT_EQ(c.dag().wire(0)[1], 2u);
  EXPECT_EQ(c.dag().wire(1).size(), 2u);
  EXPECT_EQ(c.dag().wire(1)[0], 1u);
  EXPECT_EQ(c.dag().wire(1)[1], 2u);
}
