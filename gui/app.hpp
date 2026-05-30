// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
#pragma once

#include <string>

namespace ket::gui {

// Opens a GLFW/ImGui window titled `title` with a single "Circuit" panel that
// shows the given QASM source, and runs until the user closes it. Returns a
// process exit code.
int run(const std::string& qasm_source, const std::string& title);

}  // namespace ket::gui
