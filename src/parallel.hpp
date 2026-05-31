// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
//
// Internal parallel-for over a flat index range, backed by a persistent thread
// pool (see parallelism.cpp). Not installed.
#pragma once

#include <cstddef>
#include <functional>

namespace ket::internal {

// Run `body(begin, end)` over a partition of [0, count) across the configured
// threads, blocking until every partition is done. Falls back to a single call
// when there is one thread or `count` is below the parallelization threshold.
//
// The caller must ensure the work is partition-safe: distinct sub-ranges must
// not write the same element. Every state-vector kernel satisfies this because
// each amplitude pair is owned by its lower (bit-cleared) index.
void parallel_for(std::size_t count,
                  const std::function<void(std::size_t, std::size_t)>& body);

}  // namespace ket::internal
