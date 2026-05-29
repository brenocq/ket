#include <gtest/gtest.h>
#include <ket/circuit.hpp>

TEST(Circuit, EmptyHasNoGates) {
  ket::Circuit c{2};
  EXPECT_EQ(c.n_qubits(), 2u);
  EXPECT_TRUE(c.dag().nodes().empty());
}

TEST(Circuit, BellCircuit) {
  ket::Circuit c{2};
  auto q0 = c.qubit(0);
  auto q1 = c.qubit(1);
  c.h(q0);
  c.cnot(q0, q1);

  const auto& nodes = c.dag().nodes();
  ASSERT_EQ(nodes.size(), 2u);

  EXPECT_EQ(nodes[0].gate.type, ket::GateType::H);
  ASSERT_EQ(nodes[0].gate.qubits.size(), 1u);
  EXPECT_EQ(nodes[0].gate.qubits[0].index, 0u);

  EXPECT_EQ(nodes[1].gate.type, ket::GateType::CNOT);
  ASSERT_EQ(nodes[1].gate.qubits.size(), 2u);
  EXPECT_EQ(nodes[1].gate.qubits[0].index, 0u);
  EXPECT_EQ(nodes[1].gate.qubits[1].index, 1u);
}

TEST(Circuit, AcceptsSizeTOverloads) {
  ket::Circuit c{2};
  c.h(0);
  c.cnot(0, 1);
  const auto& nodes = c.dag().nodes();
  ASSERT_EQ(nodes.size(), 2u);
  EXPECT_EQ(nodes[0].gate.type, ket::GateType::H);
  ASSERT_EQ(nodes[0].gate.qubits.size(), 1u);
  EXPECT_EQ(nodes[0].gate.qubits[0].index, 0u);
  EXPECT_EQ(nodes[1].gate.type, ket::GateType::CNOT);
  ASSERT_EQ(nodes[1].gate.qubits.size(), 2u);
  EXPECT_EQ(nodes[1].gate.qubits[0].index, 0u);
  EXPECT_EQ(nodes[1].gate.qubits[1].index, 1u);
}

TEST(Circuit, AllSingleQubitGates) {
  ket::Circuit c{1};
  auto q = c.qubit(0);
  c.h(q);
  c.x(q);
  c.z(q);
  const auto& nodes = c.dag().nodes();
  ASSERT_EQ(nodes.size(), 3u);
  EXPECT_EQ(nodes[0].gate.type, ket::GateType::H);
  EXPECT_EQ(nodes[1].gate.type, ket::GateType::X);
  EXPECT_EQ(nodes[2].gate.type, ket::GateType::Z);
}
