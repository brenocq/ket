#include <gtest/gtest.h>
#include <ket/circuit.hpp>

TEST(Circuit, EmptyHasNoGates) {
  ket::Circuit c{2};
  EXPECT_EQ(c.n_qubits(), 2u);
  EXPECT_TRUE(c.gates().empty());
}

TEST(Circuit, BellCircuit) {
  ket::Circuit c{2};
  auto q0 = c.qubit(0);
  auto q1 = c.qubit(1);
  c.h(q0);
  c.cnot(q0, q1);

  ASSERT_EQ(c.gates().size(), 2u);

  EXPECT_EQ(c.gates()[0].type, ket::GateType::H);
  ASSERT_EQ(c.gates()[0].qubits.size(), 1u);
  EXPECT_EQ(c.gates()[0].qubits[0].index, 0u);

  EXPECT_EQ(c.gates()[1].type, ket::GateType::CNOT);
  ASSERT_EQ(c.gates()[1].qubits.size(), 2u);
  EXPECT_EQ(c.gates()[1].qubits[0].index, 0u);
  EXPECT_EQ(c.gates()[1].qubits[1].index, 1u);
}

TEST(Circuit, AllSingleQubitGates) {
  ket::Circuit c{1};
  auto q = c.qubit(0);
  c.h(q);
  c.x(q);
  c.z(q);
  ASSERT_EQ(c.gates().size(), 3u);
  EXPECT_EQ(c.gates()[0].type, ket::GateType::H);
  EXPECT_EQ(c.gates()[1].type, ket::GateType::X);
  EXPECT_EQ(c.gates()[2].type, ket::GateType::Z);
}
