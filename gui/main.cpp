// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
//
// Entry point for the `ket-gui` executable: `ket-gui <file.qasm>` opens the
// circuit window (editable QASM, circuit diagram, state and qubit panels).
// Other files can be opened from the in-window File menu while it is running.
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <ket/version.hpp>

#include "app.hpp"

namespace {

std::string read_file(const std::string& path, bool& ok) {
  std::ifstream file(path);
  if (!file) {
    ok = false;
    return {};
  }
  std::ostringstream ss;
  ss << file.rdbuf();
  ok = true;
  return ss.str();
}

void usage(std::ostream& os) {
  os << "Usage: ket-gui <file.qasm>\n"
        "\n"
        "Opens a window with the editable QASM, circuit diagram, and state "
        "and\n"
        "qubit panels. Use the File menu to open other files while it runs.\n"
        "\n"
        "Options:\n"
        "  -v, --version   Print the version and exit\n"
        "  -h, --help      Show this help and exit\n";
}

}  // namespace

int main(int argc, char** argv) {
  const std::vector<std::string> args(argv + 1, argv + argc);
  if (args.empty()) {
    usage(std::cerr);
    return 1;
  }

  const std::string& arg = args[0];
  if (arg == "-h" || arg == "--help") {
    usage(std::cout);
    return 0;
  }
  if (arg == "-v" || arg == "--version") {
    std::cout << "ket-gui " << ket::version << "\n";
    return 0;
  }
  if (arg.size() > 1 && arg[0] == '-') {
    std::cerr << "error: unknown option '" << arg << "'\n";
    return 1;
  }
  if (args.size() > 1) {
    std::cerr << "error: unexpected argument '" << args[1] << "'\n";
    return 1;
  }

  bool ok = false;
  const std::string source = read_file(arg, ok);
  if (!ok) {
    std::cerr << "error: cannot open file: " << arg << "\n";
    return 2;
  }

  return ket::gui::run(source, arg);
}
