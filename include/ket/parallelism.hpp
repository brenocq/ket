// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
#pragma once

namespace ket {

// Number of threads the state-vector backend uses to apply each gate.
//
// The 2^n amplitude update inside a gate is embarrassingly parallel, so it is
// split across this many threads (within a gate; gates stay sequential). The
// threads live in a persistent pool, so there is no per-gate spawn cost, and
// gates on small states run serially regardless (the sync would cost more than
// the work).
//
// Defaults to 1 (serial, deterministic) unless the KET_NUM_THREADS environment
// variable is set. `set_num_threads(0)` selects the hardware concurrency.
void set_num_threads(unsigned threads);
unsigned num_threads();

}  // namespace ket
