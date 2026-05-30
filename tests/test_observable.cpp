#include <gtest/gtest.h>
#include <ket/circuit.hpp>
#include <ket/observable.hpp>
#include <ket/simulator.hpp>

#include <stdexcept>

namespace {
constexpr double kTol = 1e-12;
}  // namespace

TEST(Observable, SingleQubitZ) {
  EXPECT_NEAR(ket::expval(ket::run(ket::Circuit{1}), "Z"), 1.0, kTol);  // |0>
  ket::Circuit c{1};
  c.x(0);
  EXPECT_NEAR(ket::expval(ket::run(c), "Z"), -1.0, kTol);  // |1>
}

TEST(Observable, PlusState) {
  ket::Circuit c{1};
  c.h(0);  // |+>
  const ket::State s = ket::run(c);
  EXPECT_NEAR(ket::expval(s, "X"), 1.0, kTol);
  EXPECT_NEAR(ket::expval(s, "Z"), 0.0, kTol);
  EXPECT_NEAR(ket::expval(s, "Y"), 0.0, kTol);
}

TEST(Observable, YEigenstate) {
  ket::Circuit c{1};
  c.h(0);
  c.s(0);  // |+i>, the +1 eigenstate of Y
  EXPECT_NEAR(ket::expval(ket::run(c), "Y"), 1.0, kTol);
}

TEST(Observable, BellCorrelations) {
  ket::Circuit c{2};
  c.h(0);
  c.cx(0, 1);
  const ket::State s = ket::run(c);
  EXPECT_NEAR(ket::expval(s, "ZZ"), 1.0, kTol);   // qubits always agree
  EXPECT_NEAR(ket::expval(s, "XX"), 1.0, kTol);   // also correlated in X
  EXPECT_NEAR(ket::expval(s, "YY"), -1.0, kTol);  // anti-correlated in Y
  EXPECT_NEAR(ket::expval(s, "ZI"), 0.0, kTol);   // each qubit alone is random
  EXPECT_NEAR(ket::expval(s, "IZ"), 0.0, kTol);
}

TEST(Observable, RightToLeftOrdering) {
  ket::Circuit c{2};
  c.x(1);  // |10>: qubit 1 = 1, qubit 0 = 0
  const ket::State s = ket::run(c);
  EXPECT_NEAR(ket::expval(s, "ZI"), -1.0, kTol);  // Z on qubit 1 -> -1
  EXPECT_NEAR(ket::expval(s, "IZ"), 1.0, kTol);   // Z on qubit 0 -> +1
}

TEST(Observable, LengthMismatchThrows) {
  EXPECT_THROW(ket::expval(ket::run(ket::Circuit{2}), "Z"),
               std::invalid_argument);
}
