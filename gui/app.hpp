// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
#pragma once

#include <string>

namespace ket::gui {

// Opens the GLFW/ImGui window on the given QASM source, loaded from `path`
// (used for the title bar and File > Save; may be empty). Runs until the user
// closes the window. Returns a process exit code.
int run(const std::string& qasm_source, const std::string& path);

}  // namespace ket::gui
