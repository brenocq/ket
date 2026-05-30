#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <vector>

#include "commands.hpp"

namespace {

// A Bell circuit with and without a measurement step.
const std::string kBell = "qreg q[2]; h q[0]; cx q[0],q[1];";
const std::string kBellMeasured =
    "qreg q[2]; creg c[2]; h q[0]; cx q[0],q[1]; "
    "measure q[0] -> c[0]; measure q[1] -> c[1];";

std::string sample_output(const std::vector<std::string>& args) {
  std::istringstream in(kBellMeasured);
  std::ostringstream out;
  std::ostringstream err;
  ket::cli::cmd_sample(args, in, out, err);
  return out.str();
}

}  // namespace

TEST(Cli, VersionMentionsName) {
  std::ostringstream out;
  ket::cli::print_version(out);
  EXPECT_NE(out.str().find("ket-cli"), std::string::npos);
}

TEST(Cli, DrawRendersTheDiagram) {
  std::istringstream in(kBell);
  std::ostringstream out;
  std::ostringstream err;
  EXPECT_EQ(ket::cli::cmd_draw({}, in, out, err), 0);
  EXPECT_NE(out.str().find("H"), std::string::npos);
  EXPECT_NE(out.str().find("q_0:"), std::string::npos);
}

TEST(Cli, RunPrintsAmplitudes) {
  std::istringstream in(kBell);
  std::ostringstream out;
  std::ostringstream err;
  EXPECT_EQ(ket::cli::cmd_run({}, in, out, err), 0);
  EXPECT_NE(out.str().find("0.707107"), std::string::npos);
}

TEST(Cli, SampleSeedIsDeterministic) {
  const std::vector<std::string> args{"--shots", "200", "--seed", "42"};
  EXPECT_EQ(sample_output(args), sample_output(args));
}

TEST(Cli, SampleBellYieldsOnlyCorrelatedOutcomes) {
  const std::string out = sample_output({"--shots", "200", "--seed", "42"});
  EXPECT_NE(out.find("00:"), std::string::npos);
  EXPECT_NE(out.find("11:"), std::string::npos);
  EXPECT_EQ(out.find("01:"), std::string::npos);
  EXPECT_EQ(out.find("10:"), std::string::npos);
}

TEST(Cli, SampleWithoutMeasurementsErrors) {
  std::istringstream in(kBell);  // no creg / measure
  std::ostringstream out;
  std::ostringstream err;
  EXPECT_EQ(ket::cli::cmd_sample({}, in, out, err), 2);
  EXPECT_NE(err.str().find("measurement"), std::string::npos);
}

TEST(Cli, BadQasmReturnsRuntimeError) {
  std::istringstream in("qreg q[1]; bogus q[0];");
  std::ostringstream out;
  std::ostringstream err;
  EXPECT_EQ(ket::cli::cmd_draw({}, in, out, err), 2);
}

TEST(Cli, UnknownOptionIsUsageError) {
  std::istringstream in(kBell);
  std::ostringstream out;
  std::ostringstream err;
  EXPECT_EQ(ket::cli::cmd_run({"--nope"}, in, out, err), 1);
}
