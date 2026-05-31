// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
//
// GLFW + Dear ImGui (docking) host. Docked panels fill the viewport: a "Code"
// panel with the editable QASM, a "Circuit" panel that renders the parsed
// circuit (with transport controls and a playhead for step-through), a "State"
// panel showing the stepped state vector, and a "Qubits" panel of per-qubit
// Bloch spheres (ImPlot3D). A ket::Stepper drives the gate-by-gate execution.
#include "app.hpp"

#include <algorithm>
#include <cctype>
#include <cfloat>
#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdio>
#include <exception>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"  // DockBuilder*
#include "imgui_stdlib.h"    // InputTextMultiline(std::string*)
#include "implot.h"
#include "implot3d.h"

#include <GLFW/glfw3.h>

#include <ket/ket.hpp>

namespace ket::gui {
namespace {

void glfw_error_callback(int error, const char* description) {
  std::fprintf(stderr, "glfw error %d: %s\n", error, description);
}

// Short box label for a gate type (ASCII only — the default ImGui font has no
// glyphs for the dagger/box-drawing characters the ASCII printer uses).
const char* gate_name(GateType t) {
  switch (t) {
    case GateType::H:
      return "H";
    case GateType::X:
      return "X";
    case GateType::Y:
    case GateType::CY:
      return "Y";
    case GateType::Z:
      return "Z";
    case GateType::S:
      return "S";
    case GateType::Sdg:
      return "Sdg";
    case GateType::T:
      return "T";
    case GateType::Tdg:
      return "Tdg";
    case GateType::Rx:
    case GateType::CRx:
      return "Rx";
    case GateType::Ry:
    case GateType::CRy:
      return "Ry";
    case GateType::Rz:
    case GateType::CRz:
      return "Rz";
    case GateType::U:
    case GateType::CU:
      return "U";
    case GateType::CH:
      return "H";
    case GateType::CP:
      return "P";
    case GateType::Measure:
      return "M";
    default:
      return "?";
  }
}

static ImU32 rgb(float r, float g, float b) {
  return IM_COL32(static_cast<int>(r * 255.0f + 0.5f),
                  static_cast<int>(g * 255.0f + 0.5f),
                  static_cast<int>(b * 255.0f + 0.5f), 255);
}

// Fill color for a gate's box. Controlled variants share the color of the gate
// they apply (CRx is the Rx color, CH the H color, ...), so the family reads at
// a glance and the control dot is what marks it controlled.
ImU32 gate_color(GateType t) {
  switch (t) {
    case GateType::H:
    case GateType::CH:
      return rgb(0.118f, 0.565f, 1.000f);  // Dodger Blue
    case GateType::X:
    case GateType::CX:
    case GateType::CCX:
      return rgb(0.894f, 0.102f, 0.110f);  // Crimson
    case GateType::Y:
    case GateType::CY:
      return rgb(0.302f, 0.686f, 0.290f);  // Forest Green
    case GateType::Z:
    case GateType::CZ:
      return rgb(0.855f, 0.647f, 0.125f);  // Goldenrod
    case GateType::S:
      return rgb(0.251f, 0.878f, 0.816f);  // Turquoise
    case GateType::Sdg:
      return rgb(0.400f, 0.804f, 0.667f);  // Medium Aquamarine
    case GateType::T:
      return rgb(0.780f, 0.082f, 0.522f);  // Medium Violet Red
    case GateType::Tdg:
      return rgb(0.729f, 0.333f, 0.827f);  // Medium Orchid
    case GateType::Rx:
    case GateType::CRx:
      return rgb(1.000f, 0.388f, 0.278f);  // Tomato
    case GateType::Ry:
    case GateType::CRy:
      return rgb(0.180f, 0.545f, 0.341f);  // Sea Green
    case GateType::Rz:
    case GateType::CRz:
      return rgb(0.255f, 0.412f, 0.882f);  // Royal Blue
    case GateType::U:
    case GateType::CU:
      return rgb(0.576f, 0.439f, 0.859f);  // Medium Purple
    case GateType::CP:
      return rgb(1.000f, 0.498f, 0.000f);  // Orange
    case GateType::Swap:
    case GateType::CSwap:
      return rgb(0.000f, 0.808f, 0.820f);  // Dark Cyan
    case GateType::Measure:
      return rgb(0.957f, 0.643f, 0.376f);  // Sandy Brown
    default:
      return rgb(0.216f, 0.494f, 0.722f);  // Steel Blue
  }
}

// Color of a QASM gate keyword, matching its box in the circuit window, or 0 if
// `tok` is not a gate keyword (so the editor and the diagram agree at a
// glance).
ImU32 qasm_gate_color(std::string_view tok) {
  using G = GateType;
  if (tok == "h") return gate_color(G::H);
  if (tok == "x") return gate_color(G::X);
  if (tok == "y") return gate_color(G::Y);
  if (tok == "z") return gate_color(G::Z);
  if (tok == "s") return gate_color(G::S);
  if (tok == "sdg") return gate_color(G::Sdg);
  if (tok == "t") return gate_color(G::T);
  if (tok == "tdg") return gate_color(G::Tdg);
  if (tok == "rx") return gate_color(G::Rx);
  if (tok == "ry") return gate_color(G::Ry);
  if (tok == "rz") return gate_color(G::Rz);
  if (tok == "u" || tok == "u1" || tok == "u2" || tok == "u3" || tok == "p")
    return gate_color(G::U);
  if (tok == "cx") return gate_color(G::CX);
  if (tok == "cy") return gate_color(G::CY);
  if (tok == "cz") return gate_color(G::CZ);
  if (tok == "ch") return gate_color(G::CH);
  if (tok == "crx") return gate_color(G::CRx);
  if (tok == "cry") return gate_color(G::CRy);
  if (tok == "crz") return gate_color(G::CRz);
  if (tok == "cu" || tok == "cu3") return gate_color(G::CU);
  if (tok == "cp" || tok == "cu1") return gate_color(G::CP);
  if (tok == "swap") return gate_color(G::Swap);
  if (tok == "ccx") return gate_color(G::CCX);
  if (tok == "cswap") return gate_color(G::CSwap);
  if (tok == "measure") return gate_color(G::Measure);
  return 0;
}

bool qasm_keyword(std::string_view tok) {
  return tok == "OPENQASM" || tok == "include" || tok == "qreg" ||
         tok == "creg" || tok == "gate" || tok == "barrier" || tok == "reset" ||
         tok == "if";
}

// Draw QASM syntax highlighting at `origin` (the text's top-left, in screen
// space). Used as an overlay over an InputTextMultiline whose own text is drawn
// transparent, so the editor stays editable while showing color. Tokens are
// advanced by their rendered width, so the overlay lines up with the input
// regardless of the (proportional) font.
void highlight_qasm(ImDrawList* dl, ImVec2 origin, const std::string& code,
                    float line_h, ImU32 text_col) {
  const ImU32 kw_col = rgb(0.847f, 0.561f, 0.996f);   // keywords — purple
  const ImU32 num_col = rgb(0.953f, 0.706f, 0.451f);  // numbers — tan
  const ImU32 str_col = rgb(0.651f, 0.890f, 0.510f);  // strings — green
  const ImU32 com_col = rgb(0.510f, 0.533f, 0.580f);  // comments — gray

  auto alpha = [](char c) {
    return std::isalpha(static_cast<unsigned char>(c)) != 0 || c == '_';
  };
  auto ident = [](char c) {
    return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_';
  };
  auto digit = [](char c) {
    return std::isdigit(static_cast<unsigned char>(c)) != 0;
  };

  // Advance with the *raw* glyph width. ImGui::CalcTextSize() ceils its result
  // (text_size.x = IM_TRUNC(x + 0.99999f)), so summing it per token over-counts
  // by up to ~1px each and drifts right. The input's cursor uses the unrounded
  // width (ImFontCalcTextSizeEx), which is what CalcTextSizeA returns here.
  ImGuiContext& g = *ImGui::GetCurrentContext();
  ImFont* const font = g.Font;
  const float font_size = g.FontSize;

  // ImGui renders each line in a single call and truncates the start position
  // once (ImFont::RenderText does IM_TRUNC(pos.x)), then lays out glyphs from
  // there. We draw token-by-token, so truncating each token's start would drift
  // ±1px against that layout (and against the cursor). Match ImGui by
  // truncating the line origin once and snapping each token to an integer
  // offset from it.
  const float ox = IM_TRUNC(origin.x);
  float y = origin.y;
  std::size_t line_start = 0;
  while (line_start <= code.size()) {
    std::size_t line_end = code.find('\n', line_start);
    if (line_end == std::string::npos) line_end = code.size();

    float adv = 0.0f;  // running width from the line start (unrounded)
    std::size_t i = line_start;
    while (i < line_end) {
      std::size_t j = i;
      ImU32 col = text_col;
      const char c = code[i];
      if (c == '/' && i + 1 < line_end && code[i + 1] == '/') {
        j = line_end;  // line comment
        col = com_col;
      } else if (c == '"') {
        j = i + 1;
        while (j < line_end && code[j] != '"') ++j;
        if (j < line_end) ++j;
        col = str_col;
      } else if (alpha(c)) {
        while (j < line_end && ident(code[j])) ++j;
        const std::string_view tok(code.data() + i, j - i);
        const ImU32 gc = qasm_gate_color(tok);
        if (gc != 0) {
          col = gc;  // exactly the gate's color in the circuit window
        } else if (tok == "pi") {
          col = num_col;
        } else if (qasm_keyword(tok)) {
          col = kw_col;
        }
      } else if (digit(c) ||
                 (c == '.' && i + 1 < line_end && digit(code[i + 1]))) {
        while (j < line_end && (digit(code[j]) || code[j] == '.')) ++j;
        col = num_col;
      } else {
        j = i + 1;  // a single punctuation / whitespace character
      }

      const char* a = code.data() + i;
      const char* b = code.data() + j;
      dl->AddText(ImVec2(ox + IM_ROUND(adv), y), col, a, b);
      adv += font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, a, b, nullptr).x;
      i = j;
    }

    y += line_h;
    if (line_end == code.size()) break;
    line_start = line_end + 1;
  }
}

// Map a phase angle (radians) onto a cyclic hue wheel, so relative phase reads
// at a glance and wraps cleanly at +-pi.
ImU32 phase_color(double phase) {
  constexpr double two_pi = 6.283185307179586;
  float h = static_cast<float>(phase / two_pi + 0.5);  // [-pi,pi] -> [0,1)
  h -= std::floor(h);
  float r = 0.0f;
  float g = 0.0f;
  float b = 0.0f;
  ImGui::ColorConvertHSVtoRGB(h, 0.62f, 1.0f, r, g, b);
  return IM_COL32(static_cast<int>(r * 255.0f + 0.5f),
                  static_cast<int>(g * 255.0f + 0.5f),
                  static_cast<int>(b * 255.0f + 0.5f), 255);
}

// The State panel: a phase-colored probability bar chart (bar height is
// |amp|^2, color is the amplitude's phase) plus a per-amplitude table.
void render_state(const State& state, ImFont* mono_font) {
  constexpr double pi = 3.141592653589793;
  const std::size_t dim = state.size();
  if (dim == 0) {
    ImGui::TextDisabled("(no state)");
    return;
  }
  int nq = 0;
  while ((std::size_t{1} << nq) < dim) ++nq;

  double maxp = 0.0;
  std::size_t nonzero = 0;
  for (std::size_t i = 0; i < dim; ++i) {
    const double p = std::norm(state[i]);  // |amp|^2
    if (p > 1e-12) ++nonzero;
    maxp = std::max(maxp, p);
  }

  ImGui::Text("%d qubit%s · %zu amplitudes · %zu nonzero", nq,
              nq == 1 ? "" : "s", dim, nonzero);

  // Phase legend: a hue strip from -pi to +pi.
  {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 p0 = ImGui::GetCursorScreenPos();
    const float w = ImMin(200.0f, ImGui::GetContentRegionAvail().x);
    const float h = ImGui::GetTextLineHeight();
    const int seg = 48;
    for (int s = 0; s < seg; ++s) {
      const double ph = (static_cast<double>(s) / seg) * 2.0 * pi - pi;
      dl->AddRectFilled(ImVec2(p0.x + w * s / seg, p0.y),
                        ImVec2(p0.x + w * (s + 1) / seg, p0.y + h),
                        phase_color(ph));
    }
    ImGui::Dummy(ImVec2(w, h));
    ImGui::SameLine();
    ImGui::TextDisabled("phase -pi .. +pi");
  }

  // Probability bar chart, colored by phase (skipped when there are too many
  // amplitudes to draw a bar each).
  if (dim <= 4096 && maxp > 0.0) {
    if (ImPlot::BeginPlot("##amps", ImVec2(-1, 150),
                          ImPlotFlags_NoLegend | ImPlotFlags_NoMenus)) {
      ImPlot::SetupAxes("basis index", "probability",
                        ImPlotAxisFlags_NoGridLines, 0);
      ImPlot::SetupAxesLimits(-0.5, static_cast<double>(dim) - 0.5, 0.0,
                              maxp * 1.10, ImPlotCond_Always);
      ImDrawList* dl = ImPlot::GetPlotDrawList();
      ImPlot::PushPlotClipRect();
      const double halfw = dim <= 128 ? 0.42 : 0.5;
      for (std::size_t i = 0; i < dim; ++i) {
        const double p = std::norm(state[i]);
        if (p <= 1e-12) continue;
        const ImVec2 a =
            ImPlot::PlotToPixels(static_cast<double>(i) - halfw, p);
        const ImVec2 b =
            ImPlot::PlotToPixels(static_cast<double>(i) + halfw, 0.0);
        dl->AddRectFilled(a, b, phase_color(std::arg(state[i])));
      }
      ImPlot::PopPlotClipRect();
      ImPlot::EndPlot();
    }
  } else if (dim > 4096) {
    ImGui::TextDisabled("(bar chart hidden above 4096 amplitudes)");
  }

  // Per-amplitude table (monospace for aligned kets and numbers). Lists the
  // nonzero amplitudes in basis-index order, capped so huge states stay snappy.
  ImGui::PushFont(mono_font);
  const ImGuiTableFlags tflags =
      ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg |
      ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp;
  if (ImGui::BeginTable("amps_table", 4, tflags)) {
    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn("state");
    ImGui::TableSetupColumn("probability");
    ImGui::TableSetupColumn("amplitude");
    ImGui::TableSetupColumn("phase (deg)");
    ImGui::TableHeadersRow();

    const std::size_t cap = 4096;
    std::size_t shown = 0;
    char ket[72];
    for (std::size_t i = 0; i < dim && shown < cap; ++i) {
      const Complex amp = state[i];
      const double p = std::norm(amp);
      if (p <= 1e-12) continue;
      ++shown;
      ImGui::TableNextRow();

      ImGui::TableNextColumn();
      std::size_t k = 0;
      ket[k++] = '|';
      for (int b = nq - 1; b >= 0; --b) ket[k++] = ((i >> b) & 1) ? '1' : '0';
      ket[k++] = '>';
      ket[k] = '\0';
      ImGui::TextUnformatted(ket);

      ImGui::TableNextColumn();
      char ov[24];
      std::snprintf(ov, sizeof(ov), "%.4f", p);
      ImGui::ProgressBar(static_cast<float>(p),
                         ImVec2(-FLT_MIN, ImGui::GetTextLineHeight()), ov);

      ImGui::TableNextColumn();
      ImGui::Text("%+.4f %+.4fi", amp.real(), amp.imag());

      ImGui::TableNextColumn();
      const double ph = std::arg(amp);
      const ImVec2 sw = ImGui::GetCursorScreenPos();
      const float hgt = ImGui::GetTextLineHeight();
      ImGui::GetWindowDrawList()->AddRectFilled(
          sw, ImVec2(sw.x + hgt, sw.y + hgt), phase_color(ph));
      ImGui::Dummy(ImVec2(hgt, hgt));
      ImGui::SameLine();
      ImGui::Text("%+.0f", ph * 180.0 / pi);
    }
    ImGui::EndTable();
  }
  ImGui::PopFont();
}

// One qubit's reduced Bloch vector r = (<X>, <Y>, <Z>), obtained by tracing out
// the other qubits. |r| is the purity: 1 = pure (unentangled), shrinking toward
// 0 as the qubit becomes entangled with the rest (0 = maximally mixed).
struct Bloch {
  double x;
  double y;
  double z;
};

Bloch bloch_vector(const State& state, int qubit) {
  const std::size_t bit = std::size_t{1} << qubit;
  double bx = 0.0;
  double by = 0.0;
  double bz = 0.0;
  for (std::size_t j = 0; j < state.size(); ++j) {
    if (j & bit) continue;  // pair each bit-cleared index with its bit-set twin
    const Complex w = std::conj(state[j]) * state[j | bit];
    bx += 2.0 * w.real();
    by += 2.0 * w.imag();
    bz += std::norm(state[j]) - std::norm(state[j | bit]);
  }
  return {bx, by, bz};
}

// Draw one square Bloch sphere: a wireframe shell that fades as the qubit
// becomes more entangled, plus the Bloch vector as an arrow whose length is the
// purity. NoClip keeps the arrow/labels visible right up to the box edges; the
// view starts at a shared orientation but can be rotated per sphere.
void draw_bloch_sphere(const char* id, float side, const Bloch& b) {
  const ImPlot3DFlags flags = ImPlot3DFlags_CanvasOnly | ImPlot3DFlags_Equal |
                              ImPlot3DFlags_NoClip | ImPlot3DFlags_NoPan |
                              ImPlot3DFlags_NoZoom;
  if (!ImPlot3D::BeginPlot(id, ImVec2(side, side), flags)) return;
  ImPlot3D::SetupAxes(
      nullptr, nullptr, nullptr, ImPlot3DAxisFlags_NoDecorations,
      ImPlot3DAxisFlags_NoDecorations, ImPlot3DAxisFlags_NoDecorations);
  ImPlot3D::SetupAxisLimits(ImAxis3D_X, -1.3, 1.3, ImPlot3DCond_Always);
  ImPlot3D::SetupAxisLimits(ImAxis3D_Y, -1.3, 1.3, ImPlot3DCond_Always);
  ImPlot3D::SetupAxisLimits(ImAxis3D_Z, -1.3, 1.3, ImPlot3DCond_Always);
  ImPlot3D::SetupBoxRotation(22.0, -125.0, false, ImPlot3DCond_Once);

  const double r = std::sqrt(b.x * b.x + b.y * b.y + b.z * b.z);
  const float rr = static_cast<float>(std::min(r, 1.0));

  // Three great circles forming the sphere shell, fainter as purity drops.
  constexpr int N = 41;
  double cxs[N];
  double cys[N];
  double czs[N];
  ImPlot3DSpec shell;
  shell.LineColor = ImGui::ColorConvertU32ToFloat4(
      IM_COL32(150, 162, 188, static_cast<int>(26.0f + 82.0f * rr)));
  shell.LineWeight = 1.0f;
  shell.Flags = ImPlot3DItemFlags_NoLegend | ImPlot3DItemFlags_NoFit;
  for (int plane = 0; plane < 3; ++plane) {
    for (int k = 0; k < N; ++k) {
      const double t = 2.0 * 3.141592653589793 * k / (N - 1);
      const double c = std::cos(t);
      const double s = std::sin(t);
      if (plane == 0) {
        cxs[k] = c;
        cys[k] = s;
        czs[k] = 0.0;
      } else if (plane == 1) {
        cxs[k] = c;
        cys[k] = 0.0;
        czs[k] = s;
      } else {
        cxs[k] = 0.0;
        cys[k] = c;
        czs[k] = s;
      }
    }
    ImPlot3D::PlotLine("##shell", cxs, cys, czs, N, shell);
  }

  // Cardinal axes (faint), so the six basis-state labels at their tips read as
  // directions: |0>/|1> on +-Z, |+>/|-> on +-X, |i>/|-i> on +-Y.
  ImPlot3DSpec axis;
  axis.LineColor = ImGui::ColorConvertU32ToFloat4(IM_COL32(120, 130, 150, 90));
  axis.LineWeight = 1.0f;
  axis.Flags = ImPlot3DItemFlags_NoLegend | ImPlot3DItemFlags_NoFit;
  const double span[2] = {-1.0, 1.0};
  const double zero[2] = {0.0, 0.0};
  ImPlot3D::PlotLine("##ax", span, zero, zero, 2, axis);
  ImPlot3D::PlotLine("##ay", zero, span, zero, 2, axis);
  ImPlot3D::PlotLine("##az", zero, zero, span, 2, axis);

  // The Bloch vector arrow (origin -> r) with a dot at the tip.
  const double axs[2] = {0.0, b.x};
  const double ays[2] = {0.0, b.y};
  const double azs[2] = {0.0, b.z};
  const ImVec4 amber =
      ImGui::ColorConvertU32ToFloat4(IM_COL32(255, 199, 89, 255));
  ImPlot3DSpec arrow;
  arrow.LineColor = amber;
  arrow.LineWeight = 3.0f;
  arrow.Flags = ImPlot3DItemFlags_NoLegend | ImPlot3DItemFlags_NoFit;
  ImPlot3D::PlotLine("##vec", axs, ays, azs, 2, arrow);
  arrow.Marker = ImPlot3DMarker_Circle;
  arrow.MarkerSize = 4.0f;
  arrow.MarkerFillColor = amber;
  ImPlot3D::PlotScatter("##tip", &axs[1], &ays[1], &azs[1], 1, arrow);

  ImPlot3D::PlotText("|0>", 0.0, 0.0, 1.15);
  ImPlot3D::PlotText("|1>", 0.0, 0.0, -1.15);
  ImPlot3D::PlotText("|+>", 1.15, 0.0, 0.0);
  ImPlot3D::PlotText("|->", -1.15, 0.0, 0.0);
  ImPlot3D::PlotText("|i>", 0.0, 1.15, 0.0);
  ImPlot3D::PlotText("|-i>", 0.0, -1.15, 0.0);
  ImPlot3D::EndPlot();
}

// The Qubits panel: a responsive grid of equal-size square Bloch spheres, one
// per qubit. The number of spheres per row follows the panel width.
void render_qubits(const State& state) {
  const std::size_t dim = state.size();
  if (dim == 0) {
    ImGui::TextDisabled("(no state)");
    return;
  }
  int nq = 0;
  while ((std::size_t{1} << nq) < dim) ++nq;

  const float side = 150.0f;  // fixed square size; columns follow the width
  const float spacing = ImGui::GetStyle().ItemSpacing.x;
  const float avail = ImGui::GetContentRegionAvail().x;
  const int cols =
      std::max(1, static_cast<int>((avail + spacing) / (side + spacing)));
  for (int q = 0; q < nq; ++q) {
    if (q % cols != 0) ImGui::SameLine();
    ImGui::BeginGroup();
    const Bloch bv = bloch_vector(state, q);
    const double r = std::sqrt(bv.x * bv.x + bv.y * bv.y + bv.z * bv.z);
    ImGui::Text("q%d", q);
    char id[16];
    std::snprintf(id, sizeof(id), "##bq%d", q);
    draw_bloch_sphere(id, side, bv);
    ImGui::TextColored(r > 0.999 ? ImVec4(0.50f, 0.85f, 0.50f, 1.0f)
                                 : ImVec4(0.95f, 0.65f, 0.35f, 1.0f),
                       "|r| %.3f", r);
    ImGui::EndGroup();
  }
}

// Renders the circuit into a decoration-free ImPlot plot using its draw list:
// horizontal qubit wires, then gates placed into greedily-packed columns.
// `current_step` is the DAG-node index of the gate about to execute (the
// playhead); current_step == node count means the run is finished.
void render_circuit(const Circuit& circuit, std::size_t current_step) {
  const std::size_t nq = circuit.n_qubits();
  if (nq == 0) {
    ImGui::TextDisabled("(empty circuit)");
    return;
  }

  // Assign each gate a column: the leftmost free column across its qubit span.
  std::vector<int> col_of;
  int ncols = 0;
  {
    std::vector<int> free_col(nq, 0);
    for (const DagNode& node : circuit.dag().nodes()) {
      const Gate& g = node.gate;
      if (g.qubits.empty()) {
        col_of.push_back(0);
        continue;
      }
      std::size_t lo = nq - 1;
      std::size_t hi = 0;
      for (const Qubit& q : g.qubits) {
        lo = std::min(lo, q.index);
        hi = std::max(hi, q.index);
      }
      int col = 0;
      for (std::size_t i = lo; i <= hi; ++i) col = std::max(col, free_col[i]);
      for (std::size_t i = lo; i <= hi; ++i) free_col[i] = col + 1;
      col_of.push_back(col);
      ncols = std::max(ncols, col + 1);
    }
  }
  if (ncols == 0) ncols = 1;

  // Pixels per circuit cell, used to pick the initial scale. The plot uses
  // ImPlotFlags_Equal, so both axes share this scale (1 column == 1 row in
  // pixels) and double-click fit-to-data keeps that 1:1 ratio rather than
  // stretching the gate boxes.
  const float ppu = 44.0f;
  const ImVec2 avail = ImGui::GetContentRegionAvail();

  ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(0.0f, 0.0f));
  // Margin left around the data when the user double-clicks to fit-to-data.
  ImPlot::PushStyleVar(ImPlotStyleVar_FitPadding, ImVec2(0.1f, 0.1f));
  const ImPlotFlags pflags = ImPlotFlags_CanvasOnly | ImPlotFlags_Equal;
  if (!ImPlot::BeginPlot("##circuit", ImVec2(-1, -1), pflags)) {
    ImPlot::PopStyleVar(2);
    return;
  }
  ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoDecorations,
                    ImPlotAxisFlags_NoDecorations);

  // Anchor the circuit to the top-left at a fixed scale on first show, when its
  // size changes, and when the panel is resized (which also covers the docked
  // window settling its size after the first frame). Between those the axes are
  // left alone so the user's pan/zoom persists.
  static int last_ncols = -1;
  static std::size_t last_nq = static_cast<std::size_t>(-1);
  static float last_aw = -1.0f;
  static float last_ah = -1.0f;
  const bool resized = std::fabs(avail.x - last_aw) > 0.5f ||
                       std::fabs(avail.y - last_ah) > 0.5f;
  if (ncols != last_ncols || nq != last_nq || resized) {
    last_ncols = ncols;
    last_nq = nq;
    last_aw = avail.x;
    last_ah = avail.y;
    const double x_lo = -0.7;
    const double x_hi =
        x_lo + static_cast<double>(std::max(avail.x, 1.0f)) / ppu;
    const double y_hi = static_cast<double>(nq) - 1.0 + 0.7;
    const double y_lo =
        y_hi - static_cast<double>(std::max(avail.y, 1.0f)) / ppu;
    ImPlot::SetupAxisLimits(ImAxis_X1, x_lo, x_hi, ImPlotCond_Always);
    ImPlot::SetupAxisLimits(ImAxis_Y1, y_lo, y_hi, ImPlotCond_Always);
  }

  // Everything below is drawn as ImPlot items (PlotLine/PlotScatter/PlotShaded/
  // PlotText) rather than raw draw-list calls. ImPlot only knows the data
  // extents from the points its items plot, so going through items is what
  // makes double-click "fit to data" work (and ImPlot clips the items for us).
  const ImU32 wire_col = IM_COL32(150, 152, 162, 255);
  const ImU32 comp_fill = IM_COL32(120, 92, 170, 255);
  const ImU32 border_col = IM_COL32(222, 226, 236, 255);
  const ImU32 text_col = IM_COL32(245, 246, 250, 255);
  const ImU32 barrier_col = IM_COL32(130, 132, 140, 120);
  const ImU32 probe_col = IM_COL32(110, 190, 130, 200);

  auto px = [](double x, double y) { return ImPlot::PlotToPixels(x, y); };
  // q0 is drawn at the top (largest y).
  auto yq = [nq](std::size_t qi) {
    return static_cast<double>(nq) - 1.0 - static_cast<double>(qi);
  };

  // Pixels per plot unit at the current zoom, so line weights and marker sizes
  // track zoom. Gate boxes are sized in plot units, so they scale on their own.
  const ImVec2 origin = px(0.0, 0.0);
  const float sx = std::fabs(px(1.0, 0.0).x - origin.x);
  const float sy = std::fabs(px(0.0, 1.0).y - origin.y);
  const float zoom = ((sx + sy) * 0.5f) / 44.0f;
  auto thick = [zoom](float base) { return std::max(1.0f, base * zoom); };

  auto v4 = [](ImU32 c) { return ImGui::ColorConvertU32ToFloat4(c); };
  // ImPlot keys item state on label id; give each its own so they don't merge.
  int iid = 0;
  char idbuf[16];
  auto next_id = [&iid, &idbuf]() {
    std::snprintf(idbuf, sizeof(idbuf), "##i%d", iid++);
    return idbuf;
  };

  auto line = [&](double x0, double y0, double x1, double y1, ImU32 col,
                  float w) {
    const double xs[2] = {x0, x1};
    const double ys[2] = {y0, y1};
    ImPlotSpec s;
    s.LineColor = v4(col);
    s.LineWeight = w;
    s.Flags = ImPlotItemFlags_NoLegend;
    ImPlot::PlotLine(next_id(), xs, ys, 2, s);
  };
  auto marker = [&](double cx, double y, ImPlotMarker m, float size, ImU32 fill,
                    ImU32 edge, float w) {
    const double xs[1] = {cx};
    const double ys[1] = {y};
    ImPlotSpec s;
    s.Marker = m;
    s.MarkerSize = size;
    s.MarkerFillColor = v4(fill);
    s.MarkerLineColor = v4(edge);
    s.LineWeight = w;
    s.Flags = ImPlotItemFlags_NoLegend;
    ImPlot::PlotScatter(next_id(), xs, ys, 1, s);
  };
  auto text = [&](double cx, double y, const char* label, ImU32 col) {
    ImPlot::PushStyleColor(ImPlotCol_InlayText, col);
    ImPlot::PlotText(label, cx, y);
    ImPlot::PopStyleColor();
  };

  // A labeled gate box, sized in plot units (so it scales with zoom): a filled
  // rectangle (PlotShaded between a top and bottom edge), an outline, a label.
  const double bw = 0.40;  // half-width
  const double bh = 0.34;  // half-height
  auto box = [&](double cx, double y, const char* label, ImU32 fill) {
    const double xs[2] = {cx - bw, cx + bw};
    const double top[2] = {y + bh, y + bh};
    const double bot[2] = {y - bh, y - bh};
    ImPlotSpec fs;
    fs.FillColor = v4(fill);
    fs.Flags = ImPlotItemFlags_NoLegend;
    ImPlot::PlotShaded(next_id(), xs, top, bot, 2, fs);
    const double bxs[5] = {cx - bw, cx + bw, cx + bw, cx - bw, cx - bw};
    const double bys[5] = {y + bh, y + bh, y - bh, y - bh, y + bh};
    ImPlotSpec bs;
    bs.LineColor = v4(border_col);
    bs.LineWeight = thick(1.5f);
    bs.Flags = ImPlotItemFlags_NoLegend;
    ImPlot::PlotLine(next_id(), bxs, bys, 5, bs);
    // Dark label on light fills, light label on dark, so every box stays
    // legible.
    const float lum = (0.299f * ((fill >> IM_COL32_R_SHIFT) & 0xFF) +
                       0.587f * ((fill >> IM_COL32_G_SHIFT) & 0xFF) +
                       0.114f * ((fill >> IM_COL32_B_SHIFT) & 0xFF)) /
                      255.0f;
    text(cx, y, label, lum > 0.6f ? IM_COL32(20, 22, 28, 255) : text_col);
  };
  auto dot = [&](double cx, double y, ImU32 c) {
    marker(cx, y, ImPlotMarker_Circle, std::max(2.5f, 4.0f * zoom), c, c, 1.0f);
  };
  // A filled disk with a + through it — the X / CNOT-target symbol. Drawn on
  // the plot draw list (ImPlot's circle markers are only ~10-sided at this
  // radius).
  auto oplus = [&](double cx, double y, ImU32 c) {
    const float r = std::max(6.0f, 11.0f * zoom);
    const float arm = 0.5f * r;  // the + is 0.75 of the circle
    const float w = thick(2.0f);
    const ImVec2 ctr = px(cx, y);
    ImDrawList* dl = ImPlot::GetPlotDrawList();
    ImPlot::PushPlotClipRect();
    dl->AddCircleFilled(ctr, r, c, 30);
    dl->AddLine(ImVec2(ctr.x - arm, ctr.y), ImVec2(ctr.x + arm, ctr.y),
                border_col, w);
    dl->AddLine(ImVec2(ctr.x, ctr.y - arm), ImVec2(ctr.x, ctr.y + arm),
                border_col, w);
    ImPlot::PopPlotClipRect();
  };
  auto cross = [&](double cx, double y, ImU32 c) {
    marker(cx, y, ImPlotMarker_Cross, std::max(5.0f, 7.0f * zoom), c, c,
           thick(2.0f));
  };
  auto vline = [&](double cx, double ya, double yb, ImU32 c) {
    line(cx, ya, cx, yb, c, thick(2.0f));
  };

  // Wires and left-hand qubit labels.
  for (std::size_t qi = 0; qi < nq; ++qi) {
    const double y = yq(qi);
    line(0.0, y, static_cast<double>(ncols), y, wire_col, thick(1.5f));
    char buf[24];
    std::snprintf(buf, sizeof(buf), "q%zu", qi);
    text(-0.45, y, buf, text_col);
  }

  // Playhead: a soft band behind the gate about to execute, drawn before the
  // gates so they sit on top. Nothing is highlighted once the run is finished.
  if (current_step < col_of.size()) {
    const Gate& cur = circuit.dag().nodes()[current_step].gate;
    const double cxp = static_cast<double>(col_of[current_step]) + 0.5;
    std::size_t lo = 0;
    std::size_t hi = nq - 1;
    if (!cur.qubits.empty()) {
      lo = nq - 1;
      hi = 0;
      for (const Qubit& q : cur.qubits) {
        lo = std::min(lo, q.index);
        hi = std::max(hi, q.index);
      }
    }
    const double xs[2] = {cxp - 0.5, cxp + 0.5};
    const double top[2] = {yq(lo) + 0.5, yq(lo) + 0.5};
    const double bot[2] = {yq(hi) - 0.5, yq(hi) - 0.5};
    ImPlotSpec hs;
    hs.FillColor = v4(IM_COL32(255, 209, 102, 64));  // amber glow
    hs.Flags = ImPlotItemFlags_NoLegend;
    ImPlot::PlotShaded(next_id(), xs, top, bot, 2, hs);
  }

  std::size_t idx = 0;
  for (const DagNode& node : circuit.dag().nodes()) {
    const Gate& g = node.gate;
    const int col = col_of[idx++];
    if (g.qubits.empty()) continue;
    const double cx = static_cast<double>(col) + 0.5;

    std::size_t lo = nq - 1;
    std::size_t hi = 0;
    for (const Qubit& q : g.qubits) {
      lo = std::min(lo, q.index);
      hi = std::max(hi, q.index);
    }

    const ImU32 gc = gate_color(g.type);
    switch (g.type) {
      case GateType::X:  // the NOT symbol — a filled disk with a +
        oplus(cx, yq(g.qubits[0].index), gc);
        break;
      case GateType::CX:
        vline(cx, yq(g.qubits[0].index), yq(g.qubits[1].index), gc);
        dot(cx, yq(g.qubits[0].index), gc);
        oplus(cx, yq(g.qubits[1].index), gc);
        break;
      case GateType::CY:
      case GateType::CH:
      case GateType::CRx:
      case GateType::CRy:
      case GateType::CRz:
      case GateType::CU:
      case GateType::CP:
        vline(cx, yq(g.qubits[0].index), yq(g.qubits[1].index), gc);
        dot(cx, yq(g.qubits[0].index), gc);
        box(cx, yq(g.qubits[1].index), gate_name(g.type), gc);
        break;
      case GateType::CZ:
        vline(cx, yq(g.qubits[0].index), yq(g.qubits[1].index), gc);
        dot(cx, yq(g.qubits[0].index), gc);
        dot(cx, yq(g.qubits[1].index), gc);
        break;
      case GateType::Swap:
        vline(cx, yq(g.qubits[0].index), yq(g.qubits[1].index), gc);
        cross(cx, yq(g.qubits[0].index), gc);
        cross(cx, yq(g.qubits[1].index), gc);
        break;
      case GateType::CCX:
        vline(cx, yq(lo), yq(hi), gc);
        dot(cx, yq(g.qubits[0].index), gc);
        dot(cx, yq(g.qubits[1].index), gc);
        oplus(cx, yq(g.qubits[2].index), gc);
        break;
      case GateType::CSwap:
        vline(cx, yq(lo), yq(hi), gc);
        dot(cx, yq(g.qubits[0].index), gc);
        cross(cx, yq(g.qubits[1].index), gc);
        cross(cx, yq(g.qubits[2].index), gc);
        break;
      case GateType::Barrier:
        line(cx, yq(lo) + 0.5, cx, yq(hi) - 0.5, barrier_col, thick(6.0f));
        break;
      case GateType::Probe:
        line(cx, yq(lo) + 0.5, cx, yq(hi) - 0.5, probe_col, thick(2.0f));
        break;
      case GateType::Composite: {
        const double yt = yq(lo) + 0.36;  // top edge (lo is the topmost qubit)
        const double yb = yq(hi) - 0.36;  // bottom edge
        const double xs[2] = {cx - 0.36, cx + 0.36};
        const double top[2] = {yt, yt};
        const double bot[2] = {yb, yb};
        ImPlotSpec fs;
        fs.FillColor = v4(comp_fill);
        fs.Flags = ImPlotItemFlags_NoLegend;
        ImPlot::PlotShaded(next_id(), xs, top, bot, 2, fs);
        const double bxs[5] = {cx - 0.36, cx + 0.36, cx + 0.36, cx - 0.36,
                               cx - 0.36};
        const double bys[5] = {yt, yt, yb, yb, yt};
        ImPlotSpec bs;
        bs.LineColor = v4(border_col);
        bs.LineWeight = thick(1.5f);
        bs.Flags = ImPlotItemFlags_NoLegend;
        ImPlot::PlotLine(next_id(), bxs, bys, 5, bs);
        const char* label = g.label.empty() ? "circ" : g.label.c_str();
        text(cx, (yq(lo) + yq(hi)) * 0.5, label, text_col);
        break;
      }
      default:
        box(cx, yq(g.qubits[0].index), gate_name(g.type), gc);
        break;
    }
  }

  ImPlot::EndPlot();
  ImPlot::PopStyleVar(2);
}

}  // namespace

int run(const std::string& qasm_source, const std::string& title) {
  glfwSetErrorCallback(glfw_error_callback);
  if (glfwInit() == 0) {
    std::fprintf(stderr, "error: failed to initialize GLFW\n");
    return 1;
  }

  // OpenGL 3.0 + GLSL 130.
  const char* glsl_version = "#version 130";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

  GLFWwindow* window =
      glfwCreateWindow(1280, 720, title.c_str(), nullptr, nullptr);
  if (window == nullptr) {
    std::fprintf(stderr, "error: failed to create window\n");
    glfwTerminate();
    return 1;
  }
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);  // vsync

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImPlot::CreateContext();
  ImPlot3D::CreateContext();

  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  io.IniFilename = nullptr;  // rebuild the default layout each launch

  ImFont* mono_font = nullptr;
#if defined(KET_GUI_ASSETS_DIR) || defined(KET_GUI_INSTALL_ASSETS_DIR)
  // Inter for the UI and JetBrains Mono for the code editor, looking in the
  // source tree (dev builds) first and then in the install location. The first
  // font added becomes ImGui's default; a missing file falls back to the
  // built-in font.
  auto load_font = [&](const char* file) -> ImFont* {
    for (const char* dir : {
#ifdef KET_GUI_ASSETS_DIR
             KET_GUI_ASSETS_DIR,
#endif
#ifdef KET_GUI_INSTALL_ASSETS_DIR
             KET_GUI_INSTALL_ASSETS_DIR,
#endif
         }) {
      const std::string path = std::string(dir) + "/" + file;
      if (std::ifstream(path).good())
        return io.Fonts->AddFontFromFileTTF(path.c_str(), 16.0f);
    }
    return nullptr;
  };
  load_font("Inter-Regular.ttf");  // default UI font (added first)
  mono_font = load_font("JetBrainsMono-Regular.ttf");
#endif

  ImGui::StyleColorsDark();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl_version);

  // The editable QASM and the circuit parsed from it (kept across edits; a
  // parse failure leaves the previous circuit on screen alongside the error).
  std::string code = qasm_source;
  Circuit circuit{0};
  std::string parse_error;
  // Step-through state: a Stepper over the current circuit, driven by the
  // transport controls. Editing the circuit rebuilds it (resetting to step 0).
  std::optional<Stepper> stepper;
  bool playing = false;
  float play_accum = 0.0f;      // seconds accumulated toward the next auto-step
  float play_interval = 0.20f;  // seconds per gate while playing
  auto reparse = [&]() {
    try {
      circuit = from_qasm(code);
      parse_error.clear();
    } catch (const std::exception& e) {
      parse_error = e.what();
    }
    stepper.emplace(circuit);
    playing = false;
  };
  reparse();

  bool layout_done = false;

  while (glfwWindowShouldClose(window) == 0) {
    glfwPollEvents();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Advance the playhead on a timer while playing (independent of which
    // panels are visible). Catch up if several intervals elapsed in one frame.
    if (playing && stepper) {
      play_accum += ImGui::GetIO().DeltaTime;
      while (play_accum >= play_interval) {
        play_accum -= play_interval;
        if (!stepper->step()) {
          playing = false;
          break;
        }
      }
    }

    const ImGuiID dockspace_id = ImGui::DockSpaceOverViewport();
    if (!layout_done) {
      // Default layout: Circuit fills the left, Code docked on the right.
      layout_done = true;
      ImGui::DockBuilderRemoveNode(dockspace_id);
      ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
      ImGui::DockBuilderSetNodeSize(dockspace_id,
                                    ImGui::GetMainViewport()->Size);
      ImGuiID code_node = 0;
      ImGuiID circuit_node = 0;
      ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.32f,
                                  &code_node, &circuit_node);
      ImGuiID state_node = 0;
      ImGui::DockBuilderSplitNode(code_node, ImGuiDir_Down, 0.45f, &state_node,
                                  &code_node);
      ImGui::DockBuilderDockWindow("Code", code_node);
      ImGui::DockBuilderDockWindow("State", state_node);
      ImGui::DockBuilderDockWindow("Qubits", state_node);  // tab beside State
      ImGui::DockBuilderDockWindow("Circuit", circuit_node);
      ImGui::DockBuilderFinish(dockspace_id);
    }

    if (ImGui::Begin("Code")) {
      // Monospace for the editor — fixed advances keep the highlight overlay
      // exactly under the editable text (NULL keeps the UI font as a fallback).
      ImGui::PushFont(mono_font);
      // Size the editor to its content so it never scrolls internally (the Code
      // window scrolls instead). That keeps the highlight overlay — drawn at
      // the input's fixed text origin — aligned with the editable text.
      const ImGuiStyle& style = ImGui::GetStyle();
      const float line_h = ImGui::GetTextLineHeight();
      const int line_count =
          1 + static_cast<int>(std::count(code.begin(), code.end(), '\n'));
      // One line of slack so the input never grows an internal scrollbar (which
      // would scroll the text out from under the overlay).
      const float content_h =
          ImMax(static_cast<float>(line_count + 1) * line_h +
                    style.FramePadding.y * 2.0f,
                ImGui::GetContentRegionAvail().y);
      const ImVec2 cursor = ImGui::GetCursorScreenPos();
      const ImVec2 text_origin(cursor.x + style.FramePadding.x,
                               cursor.y + style.FramePadding.y);

      ImGui::PushStyleColor(ImGuiCol_Text,
                            IM_COL32(0, 0, 0, 0));  // hide raw text
      const bool changed = ImGui::InputTextMultiline(
          "##code", &code, ImVec2(-FLT_MIN, content_h),
          ImGuiInputTextFlags_AllowTabInput);
      ImGui::PopStyleColor();

      // Draw the highlight inside the input's frame, shifted by the input's own
      // horizontal scroll, so long lines stay in the box and scroll with it.
      const ImGuiID input_id = ImGui::GetItemID();
      const ImVec2 frame_min = ImGui::GetItemRectMin();
      const ImVec2 frame_max = ImGui::GetItemRectMax();
      const ImGuiInputTextState* st = ImGui::GetInputTextState(input_id);
      const float scroll_x = st != nullptr ? st->Scroll.x : 0.0f;

      // The input renders into its own child window, whose translucent frame is
      // composited after the parent — so draw onto that child's draw list to
      // land *above* the frame background instead of being washed out by it.
      ImDrawList* dl = ImGui::GetWindowDrawList();
      ImGuiContext& g = *ImGui::GetCurrentContext();
      for (ImGuiWindow* w : g.Windows) {
        if (w->ChildId == input_id) {
          dl = w->DrawList;
          break;
        }
      }

      dl->PushClipRect(ImVec2(frame_min.x + style.FramePadding.x, frame_min.y),
                       ImVec2(frame_max.x - style.FramePadding.x, frame_max.y),
                       true);
      highlight_qasm(dl, ImVec2(text_origin.x - scroll_x, text_origin.y), code,
                     line_h, IM_COL32(245, 246, 250, 255));
      dl->PopClipRect();
      if (changed) reparse();
      ImGui::PopFont();
    }
    ImGui::End();

    if (ImGui::Begin("Circuit")) {
      if (!parse_error.empty()) {
        ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.45f, 1.0f), "parse error: %s",
                           parse_error.c_str());
      }

      if (stepper) {
        Stepper& st = *stepper;
        const int n = static_cast<int>(st.size());
        const bool at_start = st.pos() == 0;
        const bool at_end = st.at_end();

        ImGui::BeginDisabled(at_start);
        if (ImGui::Button("Reset")) {
          st.reset();
          playing = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("< Prev")) {
          st.seek(st.pos() - 1);
          playing = false;
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::BeginDisabled(n == 0);
        if (ImGui::Button(playing ? "Pause" : "Play ")) {
          if (!playing && at_end) st.reset();  // replay from the start
          playing = !playing;
          play_accum = 0.0f;
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::BeginDisabled(at_end);
        if (ImGui::Button("Next >")) {
          st.step();
          playing = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("End")) {
          st.seek(st.size());
          playing = false;
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::Text("gate %d / %d", static_cast<int>(st.pos()), n);
        ImGui::SameLine();
        ImGui::TextDisabled("· %zu qubit%s%s", circuit.n_qubits(),
                            circuit.n_qubits() == 1 ? "" : "s",
                            is_clifford(circuit) ? " · Clifford" : "");

        int step_i = static_cast<int>(st.pos());
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::SliderInt("##scrub", &step_i, 0, n, "")) {
          st.seek(static_cast<std::size_t>(step_i));
          playing = false;
        }
        ImGui::Separator();
      }

      render_circuit(circuit, stepper ? stepper->pos() : 0);
    }
    ImGui::End();

    if (ImGui::Begin("State")) {
      if (stepper) {
        render_state(stepper->state(), mono_font);
      } else {
        ImGui::TextDisabled("(no circuit)");
      }
    }
    ImGui::End();

    if (ImGui::Begin("Qubits")) {
      if (stepper && circuit.n_qubits() > 0) {
        render_qubits(stepper->state());
      } else {
        ImGui::TextDisabled("(no circuit)");
      }
    }
    ImGui::End();

    ImGui::Render();
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    glClearColor(0.10f, 0.10f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImPlot3D::DestroyContext();
  ImPlot::DestroyContext();
  ImGui::DestroyContext();

  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}

}  // namespace ket::gui
