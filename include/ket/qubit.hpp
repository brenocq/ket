// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
#pragma once

#include <compare>
#include <cstddef>

namespace ket {

struct Qubit {
  std::size_t index;
  auto operator<=>(const Qubit&) const = default;
};

}  // namespace ket
