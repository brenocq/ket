#include <gtest/gtest.h>
#include <ket/circuit.hpp>
#include <ket/simulator.hpp>

#include <cmath>
#include <cstddef>

namespace {

ket::Circuit make_bell() {
  ket::Circuit bell{2, "bell"};
  bell.h(0);
  bell.cx(0, 1);
  return bell;
}

constexpr double kInvSqrt2 = 0.7071067811865476;

}  // namespace

TEST(Block, RendersAsLabeledBox) {
  ket::Circuit c{2};
  c.append(make_bell(), {0, 1});
  EXPECT_EQ(c.print(),
            "     ┌──────┐\n"
            "q_0: ┤0     ├\n"
            "     │  bell│\n"
            "q_1: ┤1     ├\n"
            "     └──────┘\n");
}

TEST(Block, DecomposeExpandsOneLevel) {
  ket::Circuit c{2};
  c.append(make_bell(), {0, 1});
  ket::Circuit d = c.decompose();

  const auto& nodes = d.dag().nodes();
  ASSERT_EQ(nodes.size(), 2u);
  EXPECT_EQ(nodes[0].gate.type, ket::GateType::H);
  EXPECT_EQ(nodes[0].gate.qubits[0].index, 0u);
  EXPECT_EQ(nodes[1].gate.type, ket::GateType::CX);
  EXPECT_EQ(nodes[1].gate.qubits[0].index, 0u);
  EXPECT_EQ(nodes[1].gate.qubits[1].index, 1u);
}

TEST(Block, DecomposeRemapsQubits) {
  ket::Circuit c{3};
  c.append(make_bell(), {1, 2});
  ket::Circuit d = c.decompose();

  const auto& nodes = d.dag().nodes();
  ASSERT_EQ(nodes.size(), 2u);
  EXPECT_EQ(nodes[0].gate.qubits[0].index, 1u);
  EXPECT_EQ(nodes[1].gate.qubits[0].index, 1u);
  EXPECT_EQ(nodes[1].gate.qubits[1].index, 2u);
}

TEST(Block, SimulatesLikeInlinedGates) {
  ket::Circuit c{2};
  c.append(make_bell(), {0, 1});
  auto s = ket::run(c);
  EXPECT_NEAR(s[0].real(), kInvSqrt2, 1e-12);
  EXPECT_NEAR(s[1].real(), 0.0, 1e-12);
  EXPECT_NEAR(s[2].real(), 0.0, 1e-12);
  EXPECT_NEAR(s[3].real(), kInvSqrt2, 1e-12);
}

TEST(Block, RunMatchesDecomposed) {
  ket::Circuit c{3};
  c.append(make_bell(), {0, 1});
  c.h(2);
  c.append(make_bell(), {1, 2}, "bell2");

  auto a = ket::run(c);
  auto b = ket::run(c.decompose());
  ASSERT_EQ(a.size(), b.size());
  for (std::size_t i = 0; i < a.size(); ++i) {
    EXPECT_NEAR(a[i].real(), b[i].real(), 1e-12);
    EXPECT_NEAR(a[i].imag(), b[i].imag(), 1e-12);
  }
}

TEST(Block, NestedComposite) {
  ket::Circuit outer{2, "outer"};
  outer.append(make_bell(), {0, 1});

  ket::Circuit c{2};
  c.append(outer, {0, 1});

  // Simulating recurses through both levels -> still a Bell state.
  auto s = ket::run(c);
  EXPECT_NEAR(s[0].real(), kInvSqrt2, 1e-12);
  EXPECT_NEAR(s[3].real(), kInvSqrt2, 1e-12);

  // decompose() expands exactly one level: outer -> bell (still composite).
  ket::Circuit d1 = c.decompose();
  ASSERT_EQ(d1.dag().nodes().size(), 1u);
  EXPECT_EQ(d1.dag().nodes()[0].gate.type, ket::GateType::Composite);

  // A second decompose reaches the primitive gates.
  ket::Circuit d2 = d1.decompose();
  ASSERT_EQ(d2.dag().nodes().size(), 2u);
  EXPECT_EQ(d2.dag().nodes()[0].gate.type, ket::GateType::H);
  EXPECT_EQ(d2.dag().nodes()[1].gate.type, ket::GateType::CX);
}

TEST(Block, NameFallsBackToSubCircuitName) {
  ket::Circuit c{2};
  c.append(make_bell(), {0, 1});  // no explicit name -> uses "bell"
  EXPECT_NE(c.print().find("bell"), std::string::npos);
}