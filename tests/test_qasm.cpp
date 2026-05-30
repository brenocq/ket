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

TEST(Qasm, ParsesUFamily) {
  // The builtin U and the qelib1 aliases u3/u2/u1 all map to ket's u gate.
  // U(pi/2,0,pi) == H, so the input register becomes a uniform superposition.
  ket::Circuit c = ket::from_qasm(R"(
    qreg q[1];
    U(pi/2,0,pi) q[0];
  )");
  auto s = ket::run(c);
  EXPECT_NEAR(s[0].real(), kInvSqrt2, 1e-12);
  EXPECT_NEAR(s[1].real(), kInvSqrt2, 1e-12);

  // u1(pi/2) is a phase: on |1> it gives e^{i pi/2}|1> = i|1>.
  ket::Circuit p = ket::from_qasm("qreg q[1]; x q[0]; u1(pi/2) q[0];");
  EXPECT_NEAR(ket::run(p)[1].imag(), 1.0, 1e-12);
}

TEST(Qasm, UGateRoundTrips) {
  ket::Circuit c{1};
  c.u(0, 1.5, 0.5, 0.25);  // arbitrary (non-pi) angles
  const std::string out = ket::to_qasm(c);
  EXPECT_NE(out.find("U("), std::string::npos);

  auto a = ket::run(c);
  auto b = ket::run(ket::from_qasm(out));
  ASSERT_EQ(a.size(), b.size());
  for (std::size_t i = 0; i < a.size(); ++i) {
    EXPECT_NEAR(a[i].real(), b[i].real(), 1e-9);
    EXPECT_NEAR(a[i].imag(), b[i].imag(), 1e-9);
  }
}

TEST(Qasm, ParsesAndEmitsCh) {
  const double inv_sqrt2 = 0.7071067811865476;
  ket::Circuit c = ket::from_qasm("qreg q[2]; x q[0]; ch q[0],q[1];");
  auto s = ket::run(c);
  EXPECT_NEAR(s[1].real(), inv_sqrt2, 1e-12);
  EXPECT_NEAR(s[3].real(), inv_sqrt2, 1e-12);
  EXPECT_NE(ket::to_qasm(c).find("ch q[0],q[1];"), std::string::npos);
}

TEST(Qasm, ParsesAndEmitsControlledRotations) {
  // crx(pi) with the control set acts like rx(pi) on the target: |01> ->
  // -i|11>.
  ket::Circuit c = ket::from_qasm("qreg q[2]; x q[0]; crx(pi) q[0],q[1];");
  EXPECT_NEAR(ket::run(c)[3].imag(), -1.0, 1e-12);
  EXPECT_NE(ket::to_qasm(c).find("crx(pi) q[0],q[1];"), std::string::npos);

  // crz round-trips through a re-parse.
  ket::Circuit z = ket::from_qasm("qreg q[2]; crz(pi/2) q[0],q[1];");
  EXPECT_NE(ket::to_qasm(z).find("crz(pi/2) q[0],q[1];"), std::string::npos);
}

TEST(Qasm, ParsesAndEmitsCy) {
  ket::Circuit c = ket::from_qasm("qreg q[2]; x q[0]; cy q[0],q[1];");
  EXPECT_NEAR(ket::run(c)[3].imag(), 1.0, 1e-12);  // i|11>
  EXPECT_NE(ket::to_qasm(c).find("cy q[0],q[1];"), std::string::npos);
}

TEST(Qasm, ParsesAndEmitsCcx) {
  // Both controls set -> target flips: |011> -> |111> (index 7).
  ket::Circuit c =
      ket::from_qasm("qreg q[3]; x q[0]; x q[1]; ccx q[0],q[1],q[2];");
  EXPECT_NEAR(ket::run(c)[7].real(), 1.0, 1e-12);
  EXPECT_NE(ket::to_qasm(c).find("ccx q[0],q[1],q[2];"), std::string::npos);
}

TEST(Qasm, ParsesAndEmitsCswap) {
  // Control set -> the two targets swap: |011> -> |101> (index 5).
  ket::Circuit c =
      ket::from_qasm("qreg q[3]; x q[0]; x q[1]; cswap q[0],q[1],q[2];");
  EXPECT_NEAR(ket::run(c)[5].real(), 1.0, 1e-12);
  EXPECT_NE(ket::to_qasm(c).find("cswap q[0],q[1],q[2];"), std::string::npos);
}

TEST(Qasm, ParsesAndEmitsSwap) {
  ket::Circuit c = ket::from_qasm("qreg q[2]; x q[0]; swap q[0],q[1];");
  EXPECT_NEAR(ket::run(c)[2].real(), 1.0, 1e-12);  // |10>
  EXPECT_NE(ket::to_qasm(c).find("swap q[0],q[1];"), std::string::npos);
}

TEST(Qasm, ThrowsOnUnsupported) {
  EXPECT_THROW(ket::from_qasm("qreg q[2]; cu3(0,0,0) q[0],q[1];"),
               std::runtime_error);
  EXPECT_THROW(ket::from_qasm("qreg q[1]; bogus q[0];"), std::runtime_error);
}
