// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
//
// GLFW + Dear ImGui (docking) host. Two docked panels fill the viewport: a
// "Code" panel with the editable QASM, and a "Circuit" panel that renders the
// parsed circuit into an ImPlot plot. ImPlot3D is initialized too, ready for
// the state plot to come.
#include "app.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <exception>
#include <string>
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

// Renders the circuit into a decoration-free ImPlot plot using its draw list:
// horizontal qubit wires, then gates placed into greedily-packed columns.
void render_circuit(const Circuit& circuit) {
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

  const ImPlotFlags pflags = ImPlotFlags_CanvasOnly | ImPlotFlags_Equal;
  if (!ImPlot::BeginPlot("##circuit", ImVec2(-1, -1), pflags)) return;
  ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoDecorations,
                    ImPlotAxisFlags_NoDecorations);

  // Fit the view on first show and whenever the circuit's size changes; on
  // other frames leave the axes untouched so the user's pan/zoom persists.
  static int last_ncols = -1;
  static std::size_t last_nq = static_cast<std::size_t>(-1);
  if (ncols != last_ncols || nq != last_nq) {
    last_ncols = ncols;
    last_nq = nq;
    ImPlot::SetupAxisLimits(ImAxis_X1, -1.4, static_cast<double>(ncols) + 0.4,
                            ImPlotCond_Always);
    ImPlot::SetupAxisLimits(ImAxis_Y1, -0.8, static_cast<double>(nq) - 0.2,
                            ImPlotCond_Always);
  }

  ImDrawList* dl = ImPlot::GetPlotDrawList();

  const ImU32 wire_col = IM_COL32(150, 152, 162, 255);
  const ImU32 box_fill = IM_COL32(70, 120, 200, 255);
  const ImU32 comp_fill = IM_COL32(120, 92, 170, 255);
  const ImU32 border_col = IM_COL32(222, 226, 236, 255);
  const ImU32 text_col = IM_COL32(245, 246, 250, 255);
  const ImU32 ctrl_col = IM_COL32(222, 226, 236, 255);
  const ImU32 barrier_col = IM_COL32(130, 132, 140, 120);
  const ImU32 probe_col = IM_COL32(110, 190, 130, 200);

  auto px = [](double x, double y) { return ImPlot::PlotToPixels(x, y); };
  // q0 is drawn at the top (largest y).
  auto yq = [nq](std::size_t qi) {
    return static_cast<double>(nq) - 1.0 - static_cast<double>(qi);
  };

  // Wires and left-hand qubit labels.
  for (std::size_t qi = 0; qi < nq; ++qi) {
    const double y = yq(qi);
    dl->AddLine(px(0.0, y), px(static_cast<double>(ncols), y), wire_col, 1.5f);
    char buf[24];
    std::snprintf(buf, sizeof(buf), "q%zu", qi);
    const ImVec2 ts = ImGui::CalcTextSize(buf);
    const ImVec2 lp = px(-0.45, y);
    dl->AddText(ImVec2(lp.x - ts.x, lp.y - ts.y * 0.5f), text_col, buf);
  }

  // Shapes are sized in pixels so labels always fit and dots stay round.
  auto box = [&](double cx, double y, const char* label) {
    const ImVec2 c = px(cx, y);
    const ImVec2 ts = ImGui::CalcTextSize(label);
    const float hw = std::max(ts.x * 0.5f + 7.0f, 13.0f);
    const float hh = ts.y * 0.5f + 6.0f;
    const ImVec2 a(c.x - hw, c.y - hh);
    const ImVec2 b(c.x + hw, c.y + hh);
    dl->AddRectFilled(a, b, box_fill, 3.0f);
    dl->AddRect(a, b, border_col, 3.0f, 0, 1.5f);
    dl->AddText(ImVec2(c.x - ts.x * 0.5f, c.y - ts.y * 0.5f), text_col, label);
  };
  auto dot = [&](double cx, double y) {
    dl->AddCircleFilled(px(cx, y), 5.0f, ctrl_col);
  };
  auto oplus = [&](double cx, double y) {
    const ImVec2 c = px(cx, y);
    const float r = 11.0f;
    dl->AddCircle(c, r, ctrl_col, 0, 2.0f);
    dl->AddLine(ImVec2(c.x - r, c.y), ImVec2(c.x + r, c.y), ctrl_col, 2.0f);
    dl->AddLine(ImVec2(c.x, c.y - r), ImVec2(c.x, c.y + r), ctrl_col, 2.0f);
  };
  auto cross = [&](double cx, double y) {
    const ImVec2 c = px(cx, y);
    const float r = 7.0f;
    dl->AddLine(ImVec2(c.x - r, c.y - r), ImVec2(c.x + r, c.y + r), ctrl_col,
                2.0f);
    dl->AddLine(ImVec2(c.x - r, c.y + r), ImVec2(c.x + r, c.y - r), ctrl_col,
                2.0f);
  };
  auto vline = [&](double cx, double ya, double yb) {
    dl->AddLine(px(cx, ya), px(cx, yb), ctrl_col, 2.0f);
  };

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

    switch (g.type) {
      case GateType::CX:
        vline(cx, yq(g.qubits[0].index), yq(g.qubits[1].index));
        dot(cx, yq(g.qubits[0].index));
        oplus(cx, yq(g.qubits[1].index));
        break;
      case GateType::CY:
      case GateType::CH:
      case GateType::CRx:
      case GateType::CRy:
      case GateType::CRz:
      case GateType::CU:
      case GateType::CP:
        vline(cx, yq(g.qubits[0].index), yq(g.qubits[1].index));
        dot(cx, yq(g.qubits[0].index));
        box(cx, yq(g.qubits[1].index), gate_name(g.type));
        break;
      case GateType::CZ:
        vline(cx, yq(g.qubits[0].index), yq(g.qubits[1].index));
        dot(cx, yq(g.qubits[0].index));
        dot(cx, yq(g.qubits[1].index));
        break;
      case GateType::Swap:
        vline(cx, yq(g.qubits[0].index), yq(g.qubits[1].index));
        cross(cx, yq(g.qubits[0].index));
        cross(cx, yq(g.qubits[1].index));
        break;
      case GateType::CCX:
        vline(cx, yq(lo), yq(hi));
        dot(cx, yq(g.qubits[0].index));
        dot(cx, yq(g.qubits[1].index));
        oplus(cx, yq(g.qubits[2].index));
        break;
      case GateType::CSwap:
        vline(cx, yq(lo), yq(hi));
        dot(cx, yq(g.qubits[0].index));
        cross(cx, yq(g.qubits[1].index));
        cross(cx, yq(g.qubits[2].index));
        break;
      case GateType::Barrier:
        dl->AddLine(px(cx, yq(lo) + 0.5), px(cx, yq(hi) - 0.5), barrier_col,
                    6.0f);
        break;
      case GateType::Probe:
        dl->AddLine(px(cx, yq(lo) + 0.5), px(cx, yq(hi) - 0.5), probe_col,
                    2.0f);
        break;
      case GateType::Composite: {
        const ImVec2 a = px(cx - 0.36, yq(lo) + 0.36);
        const ImVec2 b = px(cx + 0.36, yq(hi) - 0.36);
        dl->AddRectFilled(a, b, comp_fill, 4.0f);
        dl->AddRect(a, b, border_col, 4.0f, 0, 1.5f);
        const char* label = g.label.empty() ? "circ" : g.label.c_str();
        const ImVec2 ts = ImGui::CalcTextSize(label);
        const ImVec2 c = px(cx, (yq(lo) + yq(hi)) * 0.5);
        dl->AddText(ImVec2(c.x - ts.x * 0.5f, c.y - ts.y * 0.5f), text_col,
                    label);
        break;
      }
      default:
        box(cx, yq(g.qubits[0].index), gate_name(g.type));
        break;
    }
  }

  ImPlot::EndPlot();
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

  ImGui::StyleColorsDark();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl_version);

  // The editable QASM and the circuit parsed from it (kept across edits; a
  // parse failure leaves the previous circuit on screen alongside the error).
  std::string code = qasm_source;
  Circuit circuit{0};
  std::string parse_error;
  auto reparse = [&]() {
    try {
      circuit = from_qasm(code);
      parse_error.clear();
    } catch (const std::exception& e) {
      parse_error = e.what();
    }
  };
  reparse();

  bool layout_done = false;

  while (glfwWindowShouldClose(window) == 0) {
    glfwPollEvents();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

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
      ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.30f,
                                  &code_node, &circuit_node);
      ImGui::DockBuilderDockWindow("Code", code_node);
      ImGui::DockBuilderDockWindow("Circuit", circuit_node);
      ImGui::DockBuilderFinish(dockspace_id);
    }

    if (ImGui::Begin("Code")) {
      if (ImGui::InputTextMultiline("##code", &code,
                                    ImGui::GetContentRegionAvail())) {
        reparse();
      }
    }
    ImGui::End();

    if (ImGui::Begin("Circuit")) {
      if (!parse_error.empty()) {
        ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.45f, 1.0f), "parse error: %s",
                           parse_error.c_str());
      }
      render_circuit(circuit);
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
