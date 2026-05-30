#include <gtest/gtest.h>
#include <ket/ket.hpp>

#include <cmath>
#include <stdexcept>
#include <string>

namespace {
constexpr double kInvSqrt2 = 0.7071067811865476;
}  // namespace

TEST(Qasm, ParsesBell) {
  ket::Circuit c = ket::from_qasm(R"(
    OPENQASM 2.0;
    include "qelib1.inc";
    qreg q[2];
    h q[0];
    cx q[0],q[1];
  )");
  EXPECT_EQ(c.n_qubits(), 2u);
  auto s = ket::run(c);
  EXPECT_NEAR(s[0].real(), kInvSqrt2, 1e-12);
  EXPECT_NEAR(s[3].real(), kInvSqrt2, 1e-12);
}

TEST(Qasm, ParsesAngleExpressions) {
  // Rx(pi)|0> = -i|1>; also exercises comments.
  ket::Circuit c = ket::from_qasm("qreg q[1]; rx(pi) q[0]; // a comment\n");
  auto s = ket::run(c);
  EXPECT_NEAR(s[1].imag(), -1.0, 1e-12);
}

TEST(Qasm, MultipleRegistersFlatten) {
  // a[0] -> qubit 0, b[0] -> qubit 1.
  ket::Circuit c = ket::from_qasm("qreg a[1]; qreg b[1]; x b[0];");
  auto s = ket::run(c);
  EXPECT_NEAR(s[2].real(), 1.0, 1e-12);  // |10> (qubit 1 set) is index 2
}

TEST(Qasm, RoundTripsStably) {
  const std::string src = R"(
    OPENQASM 2.0;
    qreg q[2];
    creg c[2];
    h q[0];
    cx q[0],q[1];
    rz(pi/2) q[1];
    measure q[0] -> c[0];
    measure q[1] -> c[1];
  )";
  const std::string out = ket::to_qasm(ket::from_qasm(src));
  // Re-parsing the output and re-serializing is a fixed point.
  EXPECT_EQ(ket::to_qasm(ket::from_qasm(out)), out);
  EXPECT_NE(out.find("rz(pi/2) q[1];"), std::string::npos);
}

TEST(Qasm, CompositeBlocksAreFlattened) {
  ket::Circuit bell{2, "bell"};
  bell.h(0);
  bell.cx(0, 1);
  ket::Circuit c{2};
  c.append(bell, {0, 1});

  const std::string out = ket::to_qasm(c);
  EXPECT_NE(out.find("h q[0];"), std::string::npos);
  EXPECT_NE(out.find("cx q[0],q[1];"), std::string::npos);
  EXPECT_EQ(out.find("bell"), std::string::npos);  // no composite leaks
}

TEST(Qasm, ThrowsOnUnsupported) {
  EXPECT_THROW(ket::from_qasm("qreg q[3]; ccx q[0],q[1],q[2];"),
               std::runtime_error);
  EXPECT_THROW(ket::from_qasm("qreg q[1]; bogus q[0];"), std::runtime_error);
}
