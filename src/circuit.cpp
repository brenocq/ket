// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
#include <ket/circuit.hpp>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace ket {

Circuit::Circuit(std::size_t n_qubits, std::string name)
    : n_qubits_(n_qubits), name_(std::move(name)), dag_(n_qubits) {}

Qubit Circuit::qubit(std::size_t i) const {
  assert(i < n_qubits_);
  return Qubit{i};
}

void Circuit::h(Qubit q) {
  assert(q.index < n_qubits_);
  dag_.add(Gate{GateType::H, {q}});
}

void Circuit::x(Qubit q) {
  assert(q.index < n_qubits_);
  dag_.add(Gate{GateType::X, {q}});
}

void Circuit::y(Qubit q) {
  assert(q.index < n_qubits_);
  dag_.add(Gate{GateType::Y, {q}});
}

void Circuit::z(Qubit q) {
  assert(q.index < n_qubits_);
  dag_.add(Gate{GateType::Z, {q}});
}

void Circuit::s(Qubit q) {
  assert(q.index < n_qubits_);
  dag_.add(Gate{GateType::S, {q}});
}

void Circuit::sdg(Qubit q) {
  assert(q.index < n_qubits_);
  dag_.add(Gate{GateType::Sdg, {q}});
}

void Circuit::t(Qubit q) {
  assert(q.index < n_qubits_);
  dag_.add(Gate{GateType::T, {q}});
}

void Circuit::tdg(Qubit q) {
  assert(q.index < n_qubits_);
  dag_.add(Gate{GateType::Tdg, {q}});
}

void Circuit::cx(Qubit control, Qubit target) {
  assert(control.index < n_qubits_);
  assert(target.index < n_qubits_);
  assert(control.index != target.index);
  dag_.add(Gate{GateType::CX, {control, target}});
}

void Circuit::rx(Qubit q, double theta) {
  assert(q.index < n_qubits_);
  dag_.add(Gate{.type = GateType::Rx, .qubits = {q}, .params = {theta}});
}

void Circuit::ry(Qubit q, double theta) {
  assert(q.index < n_qubits_);
  dag_.add(Gate{.type = GateType::Ry, .qubits = {q}, .params = {theta}});
}

void Circuit::rz(Qubit q, double theta) {
  assert(q.index < n_qubits_);
  dag_.add(Gate{.type = GateType::Rz, .qubits = {q}, .params = {theta}});
}

void Circuit::u(Qubit q, double theta, double phi, double lambda) {
  assert(q.index < n_qubits_);
  dag_.add(
      Gate{.type = GateType::U, .qubits = {q}, .params = {theta, phi, lambda}});
}

void Circuit::cz(Qubit a, Qubit b) {
  assert(a.index < n_qubits_ && b.index < n_qubits_);
  assert(a.index != b.index);
  dag_.add(Gate{GateType::CZ, {a, b}});
}

void Circuit::cp(Qubit a, Qubit b, double lambda) {
  assert(a.index < n_qubits_ && b.index < n_qubits_);
  assert(a.index != b.index);
  dag_.add(Gate{.type = GateType::CP, .qubits = {a, b}, .params = {lambda}});
}

void Circuit::swap(Qubit a, Qubit b) {
  assert(a.index < n_qubits_ && b.index < n_qubits_);
  assert(a.index != b.index);
  dag_.add(Gate{GateType::Swap, {a, b}});
}

void Circuit::barrier(const std::string& label) {
  std::vector<Qubit> qubits;
  qubits.reserve(n_qubits_);
  for (std::size_t i = 0; i < n_qubits_; ++i) qubits.push_back(Qubit{i});
  dag_.add(Gate{GateType::Barrier, std::move(qubits), label});
}

void Circuit::barrier(std::initializer_list<std::size_t> qubits,
                      const std::string& label) {
  barrier(std::vector<std::size_t>(qubits), label);
}

void Circuit::barrier(const std::vector<std::size_t>& qubits,
                      const std::string& label) {
  std::vector<Qubit> qs;
  qs.reserve(qubits.size());
  for (std::size_t i : qubits) {
    assert(i < n_qubits_);
    qs.push_back(Qubit{i});
  }
  dag_.add(Gate{GateType::Barrier, std::move(qs), label});
}

void Circuit::measure(std::size_t qubit, std::size_t clbit) {
  assert(qubit < n_qubits_);
  n_clbits_ = std::max(n_clbits_, clbit + 1);
  dag_.add(Gate{
      .type = GateType::Measure, .qubits = {Qubit{qubit}}, .clbit = clbit});
}

void Circuit::measure_all() {
  for (std::size_t q = 0; q < n_qubits_; ++q) measure(q, q);
}

void Circuit::probe(const std::string& label) {
  std::string name = label;
  if (name.empty()) {
    std::size_t n = 0;
    for (const DagNode& node : dag_.nodes()) {
      if (node.gate.type == GateType::Probe) ++n;
    }
    name = "ψ" + std::to_string(n);
  }
  // Spans all qubits so it is a synchronization point in the DAG (the captured
  // state is well-defined at exactly this stage).
  std::vector<Qubit> qubits;
  qubits.reserve(n_qubits_);
  for (std::size_t i = 0; i < n_qubits_; ++i) qubits.push_back(Qubit{i});
  dag_.add(Gate{GateType::Probe, std::move(qubits), name});
}

void Circuit::append(const Circuit& sub, const std::vector<std::size_t>& qubits,
                     const std::string& name) {
  assert(qubits.size() == sub.n_qubits());
  std::vector<Qubit> qs;
  qs.reserve(qubits.size());
  for (std::size_t i : qubits) {
    assert(i < n_qubits_);
    qs.push_back(Qubit{i});
  }
  std::string block_name = !name.empty()        ? name
                           : !sub.name_.empty() ? sub.name_
                                                : "circ";
  dag_.add(Gate{GateType::Composite, std::move(qs), std::move(block_name),
                std::make_shared<const Circuit>(sub)});
}

Circuit Circuit::decompose() const {
  Circuit out{n_qubits_, name_};
  for (const DagNode& node : dag_.nodes()) {
    const Gate& g = node.gate;
    if (g.type == GateType::Composite && g.definition) {
      // Inline the definition, remapping each sub-qubit i to the parent qubit
      // recorded in g.qubits[i].
      for (const DagNode& sub_node : g.definition->dag().nodes()) {
        Gate inlined = sub_node.gate;
        for (Qubit& q : inlined.qubits) q = g.qubits[q.index];
        out.dag_.add(std::move(inlined));
      }
    } else {
      out.dag_.add(g);
    }
  }
  return out;
}

namespace {

// Each column is fixed at 5 display chars wide. Rows alternate:
// even rows are box-borders/separators, odd rows carry the qubit wire.
std::vector<std::string> default_column(std::size_t n_qubits) {
  const std::size_t height = 2 * n_qubits + 1;
  std::vector<std::string> col(height);
  for (std::size_t i = 0; i < height; ++i) {
    col[i] = (i % 2 == 1) ? "─────" : "     ";
  }
  return col;
}

std::vector<std::string> render_single(std::size_t n_qubits, std::size_t q,
                                       char label) {
  auto col = default_column(n_qubits);
  col[2 * q] = "┌───┐";
  col[2 * q + 1] = std::string("┤ ") + label + " ├";
  col[2 * q + 2] = "└───┘";
  return col;
}

// Number of display columns in a UTF-8 string (counts everything except
// continuation bytes, so e.g. "π" counts as 1, not its 2 bytes).
std::size_t display_width(const std::string& s) {
  std::size_t w = 0;
  for (char c : s) {
    if ((static_cast<unsigned char>(c) & 0xC0) != 0x80) ++w;
  }
  return w;
}

// Split a UTF-8 string into its display characters (each a lead byte plus any
// continuation bytes), e.g. "ψ0" -> {"ψ", "0"}.
std::vector<std::string> to_chars(const std::string& s) {
  std::vector<std::string> out;
  for (std::size_t i = 0; i < s.size();) {
    std::size_t len = 1;
    while (i + len < s.size() &&
           (static_cast<unsigned char>(s[i + len]) & 0xC0) == 0x80) {
      ++len;
    }
    out.push_back(s.substr(i, len));
    i += len;
  }
  return out;
}

// A single-qubit box with a multi-character label (e.g. "Rx(π/2)").
std::vector<std::string> render_box(std::size_t n_qubits, std::size_t q,
                                    const std::string& label) {
  const std::size_t height = 2 * n_qubits + 1;
  const std::size_t inner = display_width(label) + 2;  // a space on each side
  std::string dashes;
  for (std::size_t k = 0; k < inner; ++k) dashes += "─";
  std::string wire;
  for (std::size_t k = 0; k < inner + 2; ++k) wire += "─";
  const std::string blank(inner + 2, ' ');

  std::vector<std::string> col(height);
  for (std::size_t r = 0; r < height; ++r) {
    if (r == 2 * q) {
      col[r] = "┌" + dashes + "┐";
    } else if (r == 2 * q + 1) {
      col[r] = "┤ " + label + " ├";
    } else if (r == 2 * q + 2) {
      col[r] = "└" + dashes + "┘";
    } else {
      col[r] = (r % 2 == 1) ? wire : blank;
    }
  }
  return col;
}

// Angle text for a gate label. Common rational multiples of π render
// symbolically (π, π/2, 3π/4, -π, ...); anything else falls back to a decimal.
std::string format_angle(double angle) {
  if (std::fabs(angle) < 1e-12) return "0";
  const double pi = std::acos(-1.0);
  const double ratio = angle / pi;  // angle measured in units of π
  // Smallest denominator wins, so the fraction comes out already reduced.
  for (int q = 1; q <= 12; ++q) {
    const double scaled = ratio * static_cast<double>(q);
    const long p = std::lround(scaled);
    if (p != 0 && std::fabs(scaled - static_cast<double>(p)) < 1e-9) {
      const bool negative = p < 0;
      const long magnitude = negative ? -p : p;
      std::string s = (magnitude == 1) ? "π" : std::to_string(magnitude) + "π";
      if (q != 1) s += "/" + std::to_string(q);
      return negative ? "-" + s : s;
    }
  }
  std::ostringstream os;
  os << angle;
  return os.str();
}

std::vector<std::string> render_cx(std::size_t n_qubits, std::size_t control,
                                   std::size_t target) {
  auto col = default_column(n_qubits);
  if (control < target) {
    col[2 * control + 1] = "──■──";
    for (std::size_t i = 2 * control + 2; i < 2 * target; ++i) {
      col[i] = (i % 2 == 1) ? "──┼──" : "  │  ";
    }
    col[2 * target] = "┌─┴─┐";
    col[2 * target + 1] = "┤ X ├";
    col[2 * target + 2] = "└───┘";
  } else {
    col[2 * target] = "┌───┐";
    col[2 * target + 1] = "┤ X ├";
    col[2 * target + 2] = "└─┬─┘";
    for (std::size_t i = 2 * target + 3; i <= 2 * control; ++i) {
      col[i] = (i % 2 == 1) ? "──┼──" : "  │  ";
    }
    col[2 * control + 1] = "──■──";
  }
  return col;
}

// Controlled-Z: a control dot on both qubits (it is symmetric), joined by a
// vertical line.
std::vector<std::string> render_cz(std::size_t n_qubits, std::size_t a,
                                   std::size_t b) {
  auto col = default_column(n_qubits);
  const std::size_t lo = std::min(a, b);
  const std::size_t hi = std::max(a, b);
  col[2 * lo + 1] = "──■──";
  col[2 * hi + 1] = "──■──";
  for (std::size_t i = 2 * lo + 2; i < 2 * hi + 1; ++i) {
    col[i] = (i % 2 == 1) ? "──┼──" : "  │  ";
  }
  return col;
}

// SWAP: an × on each qubit, joined by a vertical line (symmetric).
std::vector<std::string> render_swap(std::size_t n_qubits, std::size_t a,
                                     std::size_t b) {
  auto col = default_column(n_qubits);
  const std::size_t lo = std::min(a, b);
  const std::size_t hi = std::max(a, b);
  col[2 * lo + 1] = "──╳──";
  col[2 * hi + 1] = "──╳──";
  for (std::size_t i = 2 * lo + 2; i < 2 * hi + 1; ++i) {
    col[i] = (i % 2 == 1) ? "──┼──" : "  │  ";
  }
  return col;
}

// A controlled gate drawn as a control dot wired to a labeled box on the target
// (e.g. controlled-phase "P(π/2)"). Generalizes the CNOT picture to any width.
std::vector<std::string> render_controlled_box(std::size_t n_qubits,
                                               std::size_t control,
                                               std::size_t target,
                                               const std::string& label) {
  const std::size_t height = 2 * n_qubits + 1;
  const std::size_t w = display_width(label) + 4;
  const std::size_t mid = w / 2;  // column of the control/connector line

  auto along = [&](const std::string& fill, const std::string& at_mid) {
    std::string s;
    for (std::size_t i = 0; i < w; ++i) s += (i == mid) ? at_mid : fill;
    return s;
  };
  auto border = [&](const std::string& left, const std::string& right,
                    bool connector, const std::string& joint) {
    std::string s = left;
    for (std::size_t i = 1; i + 1 < w; ++i)
      s += (connector && i == mid) ? joint : "─";
    s += right;
    return s;
  };

  const std::string ctrl = along("─", "■");
  const std::string conn_wire = along("─", "┼");
  const std::string conn_sep = along(" ", "│");
  const std::string box_mid = "┤ " + label + " ├";
  const bool above = control < target;

  std::vector<std::string> col(height);
  for (std::size_t r = 0; r < height; ++r) {
    col[r] = (r % 2 == 1) ? along("─", "─") : std::string(w, ' ');
  }

  col[2 * control + 1] = ctrl;
  col[2 * target] = border("┌", "┐", above, "┴");
  col[2 * target + 1] = box_mid;
  col[2 * target + 2] = border("└", "┘", !above, "┬");

  const std::size_t from = above ? 2 * control + 2 : 2 * target + 3;
  const std::size_t to = above ? 2 * target : 2 * control + 1;
  for (std::size_t i = from; i < to; ++i) {
    col[i] = (i % 2 == 1) ? conn_wire : conn_sep;
  }
  return col;
}

// A barrier is a one-column-wide dashed vertical line (░) over the spanned
// qubits. Non-spanned wires still show the horizontal wire so they stay
// connected through the column.
std::vector<std::string> render_barrier(std::size_t n_qubits,
                                        const std::vector<Qubit>& qubits) {
  const std::size_t height = 2 * n_qubits + 1;
  std::vector<bool> spanned(n_qubits, false);
  std::size_t lo = n_qubits - 1;
  std::size_t hi = 0;
  for (const Qubit q : qubits) {
    spanned[q.index] = true;
    lo = std::min(lo, q.index);
    hi = std::max(hi, q.index);
  }
  std::vector<std::string> col(height);
  for (std::size_t r = 0; r < height; ++r) {
    if (r % 2 == 1) {
      col[r] = spanned[(r - 1) / 2] ? "░" : "─";
    } else {
      col[r] = (r >= 2 * lo && r <= 2 * hi + 2) ? "░" : " ";
    }
  }
  return col;
}

// A composite block: a labeled box spanning its qubits, with each mapped wire
// annotated by its sub-circuit qubit index, and the name centered vertically.
std::vector<std::string> render_composite(std::size_t n_qubits, const Gate& g,
                                          const std::string& name,
                                          std::size_t inner_w) {
  const std::size_t height = 2 * n_qubits + 1;
  std::vector<int> sub_index(n_qubits, -1);
  std::size_t lo = n_qubits - 1;
  std::size_t hi = 0;
  for (std::size_t j = 0; j < g.qubits.size(); ++j) {
    sub_index[g.qubits[j].index] = static_cast<int>(j);
    lo = std::min(lo, g.qubits[j].index);
    hi = std::max(hi, g.qubits[j].index);
  }

  std::string dashes;
  for (std::size_t k = 0; k < inner_w; ++k) dashes += "─";
  std::string wire_full;
  for (std::size_t k = 0; k < inner_w + 2; ++k) wire_full += "─";
  const std::string spaces_full(inner_w + 2, ' ');
  const std::size_t center = lo + hi + 1;  // vertically centered row

  std::vector<std::string> col(height);
  for (std::size_t r = 0; r < height; ++r) {
    if (r < 2 * lo || r > 2 * hi + 2) {
      col[r] = (r % 2 == 1) ? wire_full : spaces_full;  // wire outside the box
    } else if (r == 2 * lo) {
      col[r] = "┌" + dashes + "┐";
    } else if (r == 2 * hi + 2) {
      col[r] = "└" + dashes + "┘";
    } else {
      std::string inner(inner_w, ' ');
      if (r == center) {  // name, right-justified
        const std::size_t start = inner_w - name.size();
        for (std::size_t k = 0; k < name.size(); ++k)
          inner[start + k] = name[k];
      }
      if (r % 2 == 1) {  // wire row: prepend the sub-qubit index
        const int j = sub_index[(r - 1) / 2];
        if (j >= 0) {
          const std::string idx = std::to_string(j);
          for (std::size_t k = 0; k < idx.size(); ++k) inner[k] = idx[k];
        }
        col[r] = "┤" + inner + "├";
      } else {
        col[r] = "│" + inner + "│";
      }
    }
  }
  return col;
}

// A measurement: an M box on the qubit, a connector down to the classical bus,
// and the target clbit index printed below. Returns the full grid height
// (quantum rows + the classical bus row + an index row).
std::vector<std::string> render_measure(std::size_t n_qubits, std::size_t qubit,
                                        std::size_t clbit) {
  const std::size_t q_height = 2 * n_qubits + 1;
  const std::size_t bus = q_height;
  const std::size_t total = q_height + 2;
  std::vector<std::string> col(total);
  for (std::size_t r = 0; r < total; ++r) {
    if (r == 2 * qubit) {
      col[r] = "┌───┐";
    } else if (r == 2 * qubit + 1) {
      col[r] = "┤ M ├";
    } else if (r == 2 * qubit + 2) {
      col[r] = "└─╥─┘";  // connector leaves the box downward
    } else if (r > 2 * qubit + 2 && r < bus) {
      col[r] = (r % 2 == 1) ? "──╫──" : "  ║  ";  // cross wires / separators
    } else if (r == bus) {
      col[r] = "══╩══";  // lands on the classical bus
    } else if (r == bus + 1) {
      std::string cell(5, ' ');
      const std::string idx = std::to_string(clbit);
      const std::size_t start = idx.size() < 5 ? (5 - idx.size()) / 2 : 0;
      for (std::size_t k = 0; k < idx.size() && start + k < 5; ++k) {
        cell[start + k] = idx[k];
      }
      col[r] = std::move(cell);
    } else {
      col[r] = (r % 2 == 1) ? "─────" : "     ";  // wire above the box
    }
  }
  return col;
}

}  // namespace

std::string Circuit::print() const {
  if (n_qubits_ == 0) return "";
  const std::size_t q_height = 2 * n_qubits_ + 1;
  const bool has_clbits = n_clbits_ > 0;
  const std::size_t total_height = has_clbits ? q_height + 2 : q_height;

  const std::size_t label_width =
      std::string("q_" + std::to_string(n_qubits_ - 1) + ":").size();
  const std::string c_label = "c: " + std::to_string(n_clbits_) + "/";
  const std::size_t prefix_width =
      has_clbits ? std::max(label_width + 1, c_label.size()) : label_width + 1;

  std::vector<std::string> prefix(total_height, std::string(prefix_width, ' '));
  for (std::size_t q = 0; q < n_qubits_; ++q) {
    std::string label = "q_" + std::to_string(q) + ":";
    label.resize(prefix_width, ' ');
    prefix[2 * q + 1] = std::move(label);
  }
  if (has_clbits) {  // classical register label, right-aligned so '/' meets bus
    prefix[q_height] =
        std::string(prefix_width - c_label.size(), ' ') + c_label;
  }

  std::vector<std::vector<std::string>> columns;
  std::vector<std::size_t> widths;  // display width of each column
  // Top-row annotations for labeled barriers: (display offset, text).
  std::vector<std::pair<std::size_t, std::string>> top_labels;
  // Bottom probe markers: (display offset, label) for the ↑ + name rows.
  std::vector<std::pair<std::size_t, std::string>> probe_marks;

  auto current_offset = [&]() {
    std::size_t offset = prefix_width;
    for (std::size_t w : widths) offset += w;
    return offset;
  };
  // Add a quantum-only column, extending it with the classical bus rows so all
  // columns share total_height.
  auto add_quantum = [&](std::vector<std::string> col, std::size_t width) {
    if (has_clbits) {
      std::string bus_cell;
      for (std::size_t k = 0; k < width; ++k) bus_cell += "═";
      col.push_back(std::move(bus_cell));
      col.push_back(std::string(width, ' '));
    }
    columns.push_back(std::move(col));
    widths.push_back(width);
  };

  if (dag_.nodes().empty()) {
    add_quantum(default_column(n_qubits_), 5);
  }
  for (const DagNode& node : dag_.nodes()) {
    const Gate& g = node.gate;
    switch (g.type) {
      case GateType::H:
        add_quantum(render_single(n_qubits_, g.qubits[0].index, 'H'), 5);
        break;
      case GateType::X:
        add_quantum(render_single(n_qubits_, g.qubits[0].index, 'X'), 5);
        break;
      case GateType::Y:
        add_quantum(render_single(n_qubits_, g.qubits[0].index, 'Y'), 5);
        break;
      case GateType::Z:
        add_quantum(render_single(n_qubits_, g.qubits[0].index, 'Z'), 5);
        break;
      case GateType::S:
        add_quantum(render_single(n_qubits_, g.qubits[0].index, 'S'), 5);
        break;
      case GateType::T:
        add_quantum(render_single(n_qubits_, g.qubits[0].index, 'T'), 5);
        break;
      case GateType::Sdg:
        add_quantum(render_box(n_qubits_, g.qubits[0].index, "S†"),
                    display_width("S†") + 4);
        break;
      case GateType::Tdg:
        add_quantum(render_box(n_qubits_, g.qubits[0].index, "T†"),
                    display_width("T†") + 4);
        break;
      case GateType::CX:
        add_quantum(render_cx(n_qubits_, g.qubits[0].index, g.qubits[1].index),
                    5);
        break;
      case GateType::Barrier:
        if (!g.label.empty())
          top_labels.emplace_back(current_offset(), g.label);
        add_quantum(render_barrier(n_qubits_, g.qubits), 1);
        break;
      case GateType::Composite: {
        const std::string nm = g.label.empty() ? "circ" : g.label;
        const std::size_t max_idx = g.qubits.empty() ? 0 : g.qubits.size() - 1;
        const std::size_t inner_w =
            std::to_string(max_idx).size() + nm.size() + 1;
        add_quantum(render_composite(n_qubits_, g, nm, inner_w), inner_w + 2);
        break;
      }
      case GateType::Measure:
        // render_measure already spans the classical rows.
        columns.push_back(
            render_measure(n_qubits_, g.qubits[0].index, g.clbit));
        widths.push_back(5);
        break;
      case GateType::Rx:
      case GateType::Ry:
      case GateType::Rz: {
        const char axis = g.type == GateType::Rx   ? 'x'
                          : g.type == GateType::Ry ? 'y'
                                                   : 'z';
        const std::string lbl =
            std::string("R") + axis + "(" + format_angle(g.params[0]) + ")";
        add_quantum(render_box(n_qubits_, g.qubits[0].index, lbl),
                    display_width(lbl) + 4);
        break;
      }
      case GateType::U: {
        const std::string lbl = "U(" + format_angle(g.params[0]) + "," +
                                format_angle(g.params[1]) + "," +
                                format_angle(g.params[2]) + ")";
        add_quantum(render_box(n_qubits_, g.qubits[0].index, lbl),
                    display_width(lbl) + 4);
        break;
      }
      case GateType::CZ:
        add_quantum(render_cz(n_qubits_, g.qubits[0].index, g.qubits[1].index),
                    5);
        break;
      case GateType::Swap:
        add_quantum(
            render_swap(n_qubits_, g.qubits[0].index, g.qubits[1].index), 5);
        break;
      case GateType::CP: {
        const std::string lbl = "P(" + format_angle(g.params[0]) + ")";
        add_quantum(render_controlled_box(n_qubits_, g.qubits[0].index,
                                          g.qubits[1].index, lbl),
                    display_width(lbl) + 4);
        break;
      }
      case GateType::Probe: {
        // A 1-wide transparent column (just wire); the ↑ + label go in the
        // bottom rows, pointing at this column.
        probe_marks.emplace_back(current_offset(), g.label);
        std::vector<std::string> col(q_height);
        for (std::size_t r = 0; r < q_height; ++r) {
          col[r] = (r % 2 == 1) ? "─" : " ";
        }
        add_quantum(std::move(col), 1);
        break;
      }
    }
  }

  std::string result;

  // A label row is emitted only when a barrier carries a label, so unlabeled
  // circuits render exactly as before. The row is pure ASCII (spaces + labels),
  // so byte offsets line up with the display columns of the grid below.
  if (!top_labels.empty()) {
    std::string row;
    for (const auto& [offset, text] : top_labels) {
      if (row.size() < offset) row.resize(offset, ' ');
      for (std::size_t k = 0; k < text.size(); ++k) {
        if (offset + k < row.size()) {
          row[offset + k] = text[k];
        } else {
          row.push_back(text[k]);
        }
      }
    }
    result += row;
    result += '\n';
  }

  for (std::size_t r = 0; r < total_height; ++r) {
    result += prefix[r];
    for (const auto& col : columns) {
      result += col[r];
    }
    result += '\n';
  }

  // Probe markers: an ↑ row and a label row, each placed at the probe's column.
  // Built per display-column (cells are full UTF-8 glyphs) so the multibyte
  // arrow and ψ labels stay aligned.
  if (!probe_marks.empty()) {
    std::vector<std::string> arrows;
    std::vector<std::string> labels;
    auto place = [](std::vector<std::string>& row, std::size_t col,
                    const std::string& glyph) {
      if (row.size() <= col) row.resize(col + 1, " ");
      row[col] = glyph;
    };
    for (const auto& [offset, label] : probe_marks) {
      place(arrows, offset, "↑");
      const std::vector<std::string> chars = to_chars(label);
      const std::size_t shift = (chars.size() - 1) / 2;  // center under arrow
      const std::size_t start = offset >= shift ? offset - shift : 0;
      for (std::size_t k = 0; k < chars.size(); ++k) {
        place(labels, start + k, chars[k]);
      }
    }
    for (const auto& cell : arrows) result += cell;
    result += '\n';
    for (const auto& cell : labels) result += cell;
    result += '\n';
  }

  return result;
}

}  // namespace ket
