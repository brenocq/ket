// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
#pragma once

#include <string>

#include <ket/circuit.hpp>

namespace ket {

// Parse a subset of OpenQASM 2.0 into a Circuit. Supported: qreg/creg
// declarations, the gates ket implements (h, x, y, z, s, sdg, t, tdg, rx, ry,
// rz, cx, cz, cp/cu1, id), measure, and barrier — with `pi` angle expressions.
// Throws std::runtime_error on anything unsupported (gate defs, classical
// control, u/u2/u3, ccx, swap, reset, ...).
Circuit from_qasm(const std::string& source);

// Serialize a Circuit to OpenQASM 2.0. Composite blocks are flattened; probes
// (a ket-only concept) are omitted.
std::string to_qasm(const Circuit& circuit);

}  // namespace ket
