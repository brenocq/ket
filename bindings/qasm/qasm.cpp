// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
//
// A small parser/serializer for a subset of OpenQASM 2.0 (see qasm.hpp).
#include <ket/qasm.hpp>

#include <cctype>
#include <cmath>
#include <cstddef>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <ket/circuit.hpp>
#include <ket/dag.hpp>
#include <ket/gate.hpp>

namespace ket {
namespace {

[[noreturn]] void fail(const std::string& msg) {
  throw std::runtime_error("qasm: " + msg);
}

bool is_space(char c) {
  return std::isspace(static_cast<unsigned char>(c)) != 0;
}
bool is_ident(char c) {
  return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_';
}

std::string trim(const std::string& s) {
  std::size_t a = 0;
  std::size_t b = s.size();
  while (a < b && is_space(s[a])) ++a;
  while (b > a && is_space(s[b - 1])) --b;
  return s.substr(a, b - a);
}

// Leading run of identifier characters (a gate/keyword name).
std::string first_word(const std::string& s) {
  std::size_t i = 0;
  while (i < s.size() && is_ident(s[i])) ++i;
  return s.substr(0, i);
}

std::vector<std::string> split(const std::string& s, char delim) {
  std::vector<std::string> out;
  std::string cur;
  for (char ch : s) {
    if (ch == delim) {
      std::string t = trim(cur);
      if (!t.empty()) out.push_back(t);
      cur.clear();
    } else {
      cur += ch;
    }
  }
  std::string t = trim(cur);
  if (!t.empty()) out.push_back(t);
  return out;
}

std::string strip_comments(const std::string& src) {
  std::string out;
  out.reserve(src.size());
  for (std::size_t i = 0; i < src.size(); ++i) {
    if (src[i] == '/' && i + 1 < src.size() && src[i + 1] == '/') {
      while (i < src.size() && src[i] != '\n') ++i;
      if (i < src.size()) out += '\n';
    } else if (src[i] == '/' && i + 1 < src.size() && src[i + 1] == '*') {
      i += 2;
      while (i + 1 < src.size() && !(src[i] == '*' && src[i + 1] == '/')) ++i;
      ++i;  // skip the closing '/'
    } else {
      out += src[i];
    }
  }
  return out;
}

// --- Angle expressions: pi, numbers, + - * /, unary signs, parentheses -------
struct ExprParser {
  const std::string& s;
  std::size_t pos = 0;
  explicit ExprParser(const std::string& str) : s(str) {}

  void skip() {
    while (pos < s.size() && is_space(s[pos])) ++pos;
  }
  bool eat(char c) {
    skip();
    if (pos < s.size() && s[pos] == c) {
      ++pos;
      return true;
    }
    return false;
  }

  double parse() {
    double v = expr();
    skip();
    if (pos != s.size()) fail("bad angle expression: " + s);
    return v;
  }
  double expr() {
    double v = term();
    for (;;) {
      if (eat('+')) {
        v += term();
      } else if (eat('-')) {
        v -= term();
      } else {
        return v;
      }
    }
  }
  double term() {
    double v = factor();
    for (;;) {
      if (eat('*')) {
        v *= factor();
      } else if (eat('/')) {
        v /= factor();
      } else {
        return v;
      }
    }
  }
  double factor() {
    skip();
    if (eat('+')) return factor();
    if (eat('-')) return -factor();
    if (eat('(')) {
      double v = expr();
      if (!eat(')')) fail("missing ')' in angle: " + s);
      return v;
    }
    if (pos + 2 <= s.size() && s.compare(pos, 2, "pi") == 0) {
      pos += 2;
      return std::acos(-1.0);
    }
    try {
      std::size_t used = 0;
      const double v = std::stod(s.substr(pos), &used);
      if (used == 0) fail("bad angle: " + s);
      pos += used;
      return v;
    } catch (const std::exception&) {
      fail("bad angle: " + s);
    }
  }
};

double eval_angle(const std::string& s) {
  if (trim(s).empty()) fail("missing angle parameter");
  ExprParser parser(s);
  return parser.parse();
}

// A flat namespace of registers (q[i] / c[j] map to a single index space).
struct Registers {
  struct Entry {
    std::size_t offset;
    std::size_t size;
  };
  std::map<std::string, Entry> entries;
  std::size_t total = 0;

  void add(const std::string& name, std::size_t n) {
    entries[name] = Entry{total, n};
    total += n;
  }
  const Entry& find(const std::string& name) const {
    auto it = entries.find(name);
    if (it == entries.end()) fail("unknown register: " + name);
    return it->second;
  }
  // Resolve "name[index]" to a flat index.
  std::size_t resolve(const std::string& token) const {
    const std::size_t lb = token.find('[');
    const std::size_t rb = token.find(']');
    if (lb == std::string::npos || rb == std::string::npos || rb < lb) {
      fail("expected indexed register, got: " + token);
    }
    const std::string name = trim(token.substr(0, lb));
    const std::size_t idx = static_cast<std::size_t>(
        std::stoul(trim(token.substr(lb + 1, rb - lb - 1))));
    const Entry& e = find(name);
    if (idx >= e.size) fail("index out of range: " + token);
    return e.offset + idx;
  }
};

void parse_decl(const std::string& statement, const std::string& keyword,
                Registers& regs) {
  const std::string rest = trim(statement.substr(keyword.size()));
  const std::size_t lb = rest.find('[');
  const std::size_t rb = rest.find(']');
  if (lb == std::string::npos || rb == std::string::npos) {
    fail("malformed declaration: " + statement);
  }
  const std::string name = trim(rest.substr(0, lb));
  const std::size_t n = static_cast<std::size_t>(
      std::stoul(trim(rest.substr(lb + 1, rb - lb - 1))));
  regs.add(name, n);
}

void apply_gate(Circuit& c, const std::string& statement,
                const Registers& qreg) {
  const std::string name = first_word(statement);
  std::size_t i = name.size();
  while (i < statement.size() && is_space(statement[i])) ++i;

  std::string params;
  if (i < statement.size() && statement[i] == '(') {
    const std::size_t close = statement.find(')', i);
    if (close == std::string::npos) fail("missing ')' in: " + statement);
    params = statement.substr(i + 1, close - i - 1);
    i = close + 1;
  }

  std::vector<std::size_t> q;
  for (const std::string& arg : split(trim(statement.substr(i)), ',')) {
    q.push_back(qreg.resolve(arg));
  }

  auto need = [&](std::size_t n) {
    if (q.size() != n)
      fail(name + ": expected " + std::to_string(n) + " qubits");
  };
  auto angles = [&]() {
    std::vector<double> a;
    for (const std::string& p : split(params, ',')) a.push_back(eval_angle(p));
    return a;
  };
  auto u_gate = [&](double theta, double phi, double lambda) {
    need(1);
    c.u(q[0], theta, phi, lambda);
  };

  if (name == "h") {
    need(1);
    c.h(q[0]);
  } else if (name == "x") {
    need(1);
    c.x(q[0]);
  } else if (name == "y") {
    need(1);
    c.y(q[0]);
  } else if (name == "z") {
    need(1);
    c.z(q[0]);
  } else if (name == "s") {
    need(1);
    c.s(q[0]);
  } else if (name == "sdg") {
    need(1);
    c.sdg(q[0]);
  } else if (name == "t") {
    need(1);
    c.t(q[0]);
  } else if (name == "tdg") {
    need(1);
    c.tdg(q[0]);
  } else if (name == "id") {
    need(1);  // identity: no-op
  } else if (name == "ch") {
    need(2);
    c.ch(q[0], q[1]);
  } else if (name == "cx") {
    need(2);
    c.cx(q[0], q[1]);
  } else if (name == "cy") {
    need(2);
    c.cy(q[0], q[1]);
  } else if (name == "cz") {
    need(2);
    c.cz(q[0], q[1]);
  } else if (name == "swap") {
    need(2);
    c.swap(q[0], q[1]);
  } else if (name == "ccx") {
    need(3);
    c.ccx(q[0], q[1], q[2]);
  } else if (name == "cswap") {
    need(3);
    c.cswap(q[0], q[1], q[2]);
  } else if (name == "rx") {
    need(1);
    c.rx(q[0], eval_angle(params));
  } else if (name == "ry") {
    need(1);
    c.ry(q[0], eval_angle(params));
  } else if (name == "rz") {
    need(1);
    c.rz(q[0], eval_angle(params));
  } else if (name == "crx") {
    need(2);
    c.crx(q[0], q[1], eval_angle(params));
  } else if (name == "cry") {
    need(2);
    c.cry(q[0], q[1], eval_angle(params));
  } else if (name == "crz") {
    need(2);
    c.crz(q[0], q[1], eval_angle(params));
  } else if (name == "cp" || name == "cu1") {
    need(2);
    c.cp(q[0], q[1], eval_angle(params));
  } else if (name == "U" || name == "u" || name == "u3") {
    const std::vector<double> a = angles();
    if (a.size() != 3) fail(name + ": expected 3 angles");
    u_gate(a[0], a[1], a[2]);
  } else if (name == "u2") {
    const std::vector<double> a = angles();
    if (a.size() != 2) fail("u2: expected 2 angles");
    u_gate(std::acos(-1.0) / 2.0, a[0], a[1]);  // U(pi/2, phi, lambda)
  } else if (name == "u1" || name == "p" || name == "phase") {
    const std::vector<double> a = angles();
    if (a.size() != 1) fail(name + ": expected 1 angle");
    u_gate(0.0, 0.0, a[0]);  // U(0, 0, lambda)
  } else {
    fail("unsupported gate: " + name);
  }
}

}  // namespace

Circuit from_qasm(const std::string& source) {
  const std::vector<std::string> statements =
      split(strip_comments(source), ';');

  // Pass 1: register declarations (so the qubit count is known up front).
  Registers qreg;
  Registers creg;
  for (const std::string& raw : statements) {
    const std::string st = trim(raw);
    const std::string head = first_word(st);
    if (head == "qreg")
      parse_decl(st, "qreg", qreg);
    else if (head == "creg")
      parse_decl(st, "creg", creg);
  }

  Circuit c{qreg.total};

  // Pass 2: operations.
  for (const std::string& raw : statements) {
    const std::string st = trim(raw);
    if (st.empty()) continue;
    const std::string head = first_word(st);

    if (head == "OPENQASM" || head == "include" || head == "qreg" ||
        head == "creg") {
      continue;
    }
    if (head == "gate" || head == "opaque" || head == "if") {
      fail("unsupported construct: " + head);
    }
    if (head == "measure") {
      const std::size_t arrow = st.find("->");
      if (arrow == std::string::npos) fail("measure needs '->': " + st);
      const std::size_t qb = qreg.resolve(trim(st.substr(7, arrow - 7)));
      const std::size_t cb = creg.resolve(trim(st.substr(arrow + 2)));
      c.measure(qb, cb);
    } else if (head == "barrier") {
      std::vector<std::size_t> qubits;
      for (const std::string& arg : split(trim(st.substr(7)), ',')) {
        if (arg.find('[') == std::string::npos) {  // whole register
          const Registers::Entry& e = qreg.find(arg);
          for (std::size_t k = 0; k < e.size; ++k)
            qubits.push_back(e.offset + k);
        } else {
          qubits.push_back(qreg.resolve(arg));
        }
      }
      c.barrier(qubits);
    } else {
      apply_gate(c, st, qreg);
    }
  }

  return c;
}

namespace {

// Render an angle as a `pi` expression when it is a small rational multiple of
// pi (pi/2, 3*pi/4, -pi, ...), otherwise a decimal. Round-trips through
// from_qasm.
std::string qasm_angle(double angle) {
  if (std::fabs(angle) < 1e-12) return "0";
  const double pi = std::acos(-1.0);
  const double ratio = angle / pi;
  for (int den = 1; den <= 12; ++den) {
    const double scaled = ratio * static_cast<double>(den);
    const long num = std::lround(scaled);
    if (num != 0 && std::fabs(scaled - static_cast<double>(num)) < 1e-9) {
      const bool negative = num < 0;
      const long mag = negative ? -num : num;
      std::string s = (mag == 1) ? "pi" : std::to_string(mag) + "*pi";
      if (den != 1) s += "/" + std::to_string(den);
      return negative ? "-" + s : s;
    }
  }
  std::ostringstream os;
  os.precision(15);
  os << angle;
  return os.str();
}

std::string single_qubit_name(GateType type) {
  switch (type) {
    case GateType::H:
      return "h";
    case GateType::X:
      return "x";
    case GateType::Y:
      return "y";
    case GateType::Z:
      return "z";
    case GateType::S:
      return "s";
    case GateType::Sdg:
      return "sdg";
    case GateType::T:
      return "t";
    case GateType::Tdg:
      return "tdg";
    default:
      return "";
  }
}

bool has_composite(const Circuit& c) {
  for (const DagNode& node : c.dag().nodes()) {
    if (node.gate.type == GateType::Composite) return true;
  }
  return false;
}

}  // namespace

std::string to_qasm(const Circuit& circuit) {
  Circuit flat = circuit;
  while (has_composite(flat)) flat = flat.decompose();

  std::ostringstream os;
  os << "OPENQASM 2.0;\n";
  os << "include \"qelib1.inc\";\n";
  os << "qreg q[" << flat.n_qubits() << "];\n";
  if (flat.n_clbits() > 0) os << "creg c[" << flat.n_clbits() << "];\n";

  for (const DagNode& node : flat.dag().nodes()) {
    const Gate& g = node.gate;
    switch (g.type) {
      case GateType::H:
      case GateType::X:
      case GateType::Y:
      case GateType::Z:
      case GateType::S:
      case GateType::Sdg:
      case GateType::T:
      case GateType::Tdg:
        os << single_qubit_name(g.type) << " q[" << g.qubits[0].index << "];\n";
        break;
      case GateType::Rx:
      case GateType::Ry:
      case GateType::Rz: {
        const char axis = g.type == GateType::Rx   ? 'x'
                          : g.type == GateType::Ry ? 'y'
                                                   : 'z';
        os << "r" << axis << "(" << qasm_angle(g.params[0]) << ") q["
           << g.qubits[0].index << "];\n";
        break;
      }
      case GateType::U:
        os << "U(" << qasm_angle(g.params[0]) << "," << qasm_angle(g.params[1])
           << "," << qasm_angle(g.params[2]) << ") q[" << g.qubits[0].index
           << "];\n";
        break;
      case GateType::CH:
        os << "ch q[" << g.qubits[0].index << "],q[" << g.qubits[1].index
           << "];\n";
        break;
      case GateType::CX:
        os << "cx q[" << g.qubits[0].index << "],q[" << g.qubits[1].index
           << "];\n";
        break;
      case GateType::CY:
        os << "cy q[" << g.qubits[0].index << "],q[" << g.qubits[1].index
           << "];\n";
        break;
      case GateType::CZ:
        os << "cz q[" << g.qubits[0].index << "],q[" << g.qubits[1].index
           << "];\n";
        break;
      case GateType::CRx:
        os << "crx(" << qasm_angle(g.params[0]) << ") q[" << g.qubits[0].index
           << "],q[" << g.qubits[1].index << "];\n";
        break;
      case GateType::CRy:
        os << "cry(" << qasm_angle(g.params[0]) << ") q[" << g.qubits[0].index
           << "],q[" << g.qubits[1].index << "];\n";
        break;
      case GateType::CRz:
        os << "crz(" << qasm_angle(g.params[0]) << ") q[" << g.qubits[0].index
           << "],q[" << g.qubits[1].index << "];\n";
        break;
      case GateType::CP:
        os << "cp(" << qasm_angle(g.params[0]) << ") q[" << g.qubits[0].index
           << "],q[" << g.qubits[1].index << "];\n";
        break;
      case GateType::Swap:
        os << "swap q[" << g.qubits[0].index << "],q[" << g.qubits[1].index
           << "];\n";
        break;
      case GateType::CCX:
        os << "ccx q[" << g.qubits[0].index << "],q[" << g.qubits[1].index
           << "],q[" << g.qubits[2].index << "];\n";
        break;
      case GateType::CSwap:
        os << "cswap q[" << g.qubits[0].index << "],q[" << g.qubits[1].index
           << "],q[" << g.qubits[2].index << "];\n";
        break;
      case GateType::Measure:
        os << "measure q[" << g.qubits[0].index << "] -> c[" << g.clbit
           << "];\n";
        break;
      case GateType::Barrier: {
        os << "barrier ";
        for (std::size_t k = 0; k < g.qubits.size(); ++k) {
          os << (k ? "," : "") << "q[" << g.qubits[k].index << "]";
        }
        os << ";\n";
        break;
      }
      case GateType::Probe:      // ket-only, no QASM equivalent
      case GateType::Composite:  // already flattened
        break;
    }
  }
  return os.str();
}

}  // namespace ket
