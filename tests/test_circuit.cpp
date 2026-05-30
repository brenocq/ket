#include <gtest/gtest.h>
#include <ket/circuit.hpp>

#include <stdexcept>

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
  c.cx(q0, q1);

  const auto& nodes = c.dag().nodes();
  ASSERT_EQ(nodes.size(), 2u);

  EXPECT_EQ(nodes[0].gate.type, ket::GateType::H);
  ASSERT_EQ(nodes[0].gate.qubits.size(), 1u);
  EXPECT_EQ(nodes[0].gate.qubits[0].index, 0u);

  EXPECT_EQ(nodes[1].gate.type, ket::GateType::CX);
  ASSERT_EQ(nodes[1].gate.qubits.size(), 2u);
  EXPECT_EQ(nodes[1].gate.qubits[0].index, 0u);
  EXPECT_EQ(nodes[1].gate.qubits[1].index, 1u);
}

TEST(Circuit, AcceptsSizeTOverloads) {
  ket::Circuit c{2};
  c.h(0);
  c.cx(0, 1);
  const auto& nodes = c.dag().nodes();
  ASSERT_EQ(nodes.size(), 2u);
  EXPECT_EQ(nodes[0].gate.type, ket::GateType::H);
  ASSERT_EQ(nodes[0].gate.qubits.size(), 1u);
  EXPECT_EQ(nodes[0].gate.qubits[0].index, 0u);
  EXPECT_EQ(nodes[1].gate.type, ket::GateType::CX);
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

TEST(Circuit, OutOfRangeQubitThrows) {
  ket::Circuit c{2};
  EXPECT_THROW(c.h(5), std::out_of_range);
  EXPECT_THROW(c.cx(0, 5), std::out_of_range);
  EXPECT_THROW(c.qubit(2), std::out_of_range);
  EXPECT_THROW(c.measure(2, 0), std::out_of_range);
  EXPECT_NO_THROW(c.cx(0, 1));  // valid input is unaffected
}

TEST(Circuit, RepeatedQubitThrows) {
  ket::Circuit c{3};
  EXPECT_THROW(c.cx(1, 1), std::invalid_argument);
  EXPECT_THROW(c.ccx(0, 1, 1), std::invalid_argument);
  EXPECT_THROW(c.cswap(0, 2, 2), std::invalid_argument);
}

TEST(Circuit, AppendArityMismatchThrows) {
  ket::Circuit sub{2};
  sub.h(0);
  ket::Circuit c{3};
  EXPECT_THROW(c.append(sub, {0, 1, 2}), std::invalid_argument);  // too many
  EXPECT_THROW(c.append(sub, {0}), std::invalid_argument);        // too few
  EXPECT_THROW(c.append(sub, {0, 5}), std::out_of_range);  // bad target qubit
  EXPECT_NO_THROW(c.append(sub, {0, 2}));
}
