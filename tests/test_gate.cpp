#include <gtest/gtest.h>
#include <ket/gate.hpp>

TEST(Gate, ConstructHadamard) {
  ket::Gate g{ket::GateType::H, {ket::Qubit{0}}};
  EXPECT_EQ(g.type, ket::GateType::H);
  ASSERT_EQ(g.qubits.size(), 1u);
  EXPECT_EQ(g.qubits[0].index, 0u);
}

TEST(Gate, ConstructCNOT) {
  ket::Gate g{ket::GateType::CX, {ket::Qubit{0}, ket::Qubit{1}}};
  EXPECT_EQ(g.type, ket::GateType::CX);
  ASSERT_EQ(g.qubits.size(), 2u);
  EXPECT_EQ(g.qubits[0].index, 0u);
  EXPECT_EQ(g.qubits[1].index, 1u);
}
