// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
//
// Entry point for the `ket-cli` executable: parse the command word and
// dispatch to the matching subcommand.
#include <iostream>
#include <string>
#include <vector>

#include "commands.hpp"

int main(int argc, char** argv) {
  const std::vector<std::string> args(argv + 1, argv + argc);
  if (args.empty()) {
    ket::cli::print_usage(std::cerr);
    return 1;
  }

  const std::string& command = args[0];
  const std::vector<std::string> rest(args.begin() + 1, args.end());

  if (command == "-v" || command == "--version") {
    ket::cli::print_version(std::cout);
    return 0;
  }
  if (command == "-h" || command == "--help" || command == "help") {
    ket::cli::print_usage(std::cout);
    return 0;
  }
  if (command == "draw") {
    return ket::cli::cmd_draw(rest, std::cin, std::cout, std::cerr);
  }
  if (command == "run") {
    return ket::cli::cmd_run(rest, std::cin, std::cout, std::cerr);
  }
  if (command == "sample") {
    return ket::cli::cmd_sample(rest, std::cin, std::cout, std::cerr);
  }

  std::cerr << "error: unknown command '" << command << "'\n\n";
  ket::cli::print_usage(std::cerr);
  return 1;
}
