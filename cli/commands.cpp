// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
//
// Implementations of the `ket-cli` subcommands.
#include "commands.hpp"

#include <algorithm>
#include <cstddef>
#include <exception>
#include <fstream>
#include <istream>
#include <map>
#include <ostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <ket/ket.hpp>

namespace ket::cli {
namespace {

std::string read_all(std::istream& in) {
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

// Reads the QASM source: from `path`, or from `in` when `path` is empty or "-".
std::string read_source(const std::string& path, std::istream& in) {
  if (path.empty() || path == "-") return read_all(in);
  std::ifstream file(path);
  if (!file) throw std::runtime_error("cannot open file: " + path);
  return read_all(file);
}

// True for tokens that look like an option (start with '-', but not the lone
// "-" stdin marker).
bool is_option(const std::string& a) { return a.size() > 1 && a[0] == '-'; }

// Pulls the single optional positional file argument out of `args`, rejecting
// unknown options and extra positionals. Returns false on error (message
// already written to `err`).
bool take_file_arg(const std::vector<std::string>& args, const char* usage,
                   std::ostream& out, std::ostream& err, std::string& file,
                   bool& handled) {
  handled = false;
  for (const std::string& a : args) {
    if (a == "-h" || a == "--help") {
      out << usage;
      handled = true;
      return true;
    }
    if (is_option(a)) {
      err << "error: unknown option '" << a << "'\n";
      return false;
    }
    if (!file.empty()) {
      err << "error: unexpected argument '" << a << "'\n";
      return false;
    }
    file = a;
  }
  return true;
}

}  // namespace

int cmd_draw(const std::vector<std::string>& args, std::istream& in,
             std::ostream& out, std::ostream& err) {
  static const char* kUsage = "Usage: ket-cli draw [file]\n";
  std::string file;
  bool handled = false;
  if (!take_file_arg(args, kUsage, out, err, file, handled)) return 1;
  if (handled) return 0;
  try {
    const Circuit c = from_qasm(read_source(file, in));
    out << c.print();
  } catch (const std::exception& e) {
    err << "error: " << e.what() << "\n";
    return 2;
  }
  return 0;
}

int cmd_run(const std::vector<std::string>& args, std::istream& in,
            std::ostream& out, std::ostream& err) {
  static const char* kUsage = "Usage: ket-cli run [file]\n";
  std::string file;
  bool handled = false;
  if (!take_file_arg(args, kUsage, out, err, file, handled)) return 1;
  if (handled) return 0;
  try {
    const Circuit c = from_qasm(read_source(file, in));
    out << run(c).print();
  } catch (const std::exception& e) {
    err << "error: " << e.what() << "\n";
    return 2;
  }
  return 0;
}

int cmd_sample(const std::vector<std::string>& args, std::istream& in,
               std::ostream& out, std::ostream& err) {
  static const char* kUsage =
      "Usage: ket-cli sample [file] [--shots N] [--seed S]\n";
  std::string file;
  long shots = 1024;
  bool has_seed = false;
  unsigned long seed = 0;

  for (std::size_t i = 0; i < args.size(); ++i) {
    const std::string& a = args[i];
    if (a == "-h" || a == "--help") {
      out << kUsage;
      return 0;
    }
    if (a == "--shots" || a == "--seed") {
      if (++i >= args.size()) {
        err << "error: " << a << " requires a value\n";
        return 1;
      }
      try {
        if (a == "--shots") {
          shots = std::stol(args[i]);
        } else {
          seed = std::stoul(args[i]);
          has_seed = true;
        }
      } catch (const std::exception&) {
        err << "error: invalid value for " << a << ": '" << args[i] << "'\n";
        return 1;
      }
    } else if (is_option(a)) {
      err << "error: unknown option '" << a << "'\n";
      return 1;
    } else if (file.empty()) {
      file = a;
    } else {
      err << "error: unexpected argument '" << a << "'\n";
      return 1;
    }
  }

  if (shots <= 0) {
    err << "error: --shots must be positive\n";
    return 1;
  }

  try {
    const Circuit c = from_qasm(read_source(file, in));
    if (c.n_clbits() == 0) {
      err << "error: circuit has no measurements (add `measure` or "
             "`measure_all`)\n";
      return 2;
    }

    std::mt19937 rng(has_seed ? static_cast<std::mt19937::result_type>(seed)
                              : std::random_device{}());
    std::map<std::string, long> counts;
    for (long s = 0; s < shots; ++s) {
      const std::vector<int> creg = sample(c, rng);
      std::string bits;  // most-significant classical bit first
      for (std::size_t k = creg.size(); k-- > 0;) {
        bits += creg[k] ? '1' : '0';
      }
      ++counts[bits];
    }

    long max_count = 0;
    for (const auto& [bits, count] : counts) {
      max_count = std::max(max_count, count);
    }
    constexpr long kBarWidth = 32;
    for (const auto& [bits, count] : counts) {
      const long len = max_count > 0 ? count * kBarWidth / max_count : 0;
      std::string bar;
      for (long b = 0; b < len; ++b) bar += "█";
      out << bits << ": " << count << " " << bar << "\n";
    }
  } catch (const std::exception& e) {
    err << "error: " << e.what() << "\n";
    return 2;
  }
  return 0;
}

void print_version(std::ostream& out) { out << "ket-cli " << version << "\n"; }

void print_usage(std::ostream& out) {
  out << "ket-cli — command-line interface for the ket quantum library\n"
         "\n"
         "Usage:\n"
         "  ket-cli <command> [options] [file]\n"
         "  ket-cli --version\n"
         "  ket-cli --help\n"
         "\n"
         "Commands:\n"
         "  draw [file]               Render the circuit diagram (ASCII)\n"
         "  run [file]                Simulate and print the final state "
         "vector\n"
         "  sample [file] [options]   Sample measurement outcomes as a "
         "histogram\n"
         "\n"
         "Sample options:\n"
         "  --shots N                 Number of shots (default: 1024)\n"
         "  --seed S                  Seed the RNG for reproducible sampling\n"
         "\n"
         "Input:\n"
         "  [file] is an OpenQASM 2.0 program; if omitted or '-', read from "
         "stdin.\n"
         "\n"
         "Options:\n"
         "  -v, --version             Print the version and exit\n"
         "  -h, --help                Show this help and exit\n";
}

}  // namespace ket::cli
