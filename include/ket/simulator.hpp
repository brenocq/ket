// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
#pragma once

#include <string>
#include <utility>
#include <vector>

#include <ket/state.hpp>

namespace ket {

class Circuit;

State run(const Circuit& circuit);

// The result of a probed run: the final state plus the states captured at each
// probe(), in circuit order.
struct ProbeRun {
  State final;
  std::vector<std::pair<std::string, State>> probes;
};

ProbeRun run_with_probes(const Circuit& circuit);

}  // namespace ket
