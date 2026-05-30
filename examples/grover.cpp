// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
//
// Grover's search: find a single "marked" item in an unstructured database of
// N = 2^n entries with only ~(pi/4)*sqrt(N) oracle queries, a quadratic speedup
// over the ~N/2 a classical search needs.
//
// The circuit is: put the n search qubits in a uniform superposition, then
// repeat { oracle, diffusion } the optimal number of times. Each round nudges
// amplitude toward the marked item; measuring then returns it with high
// probability. The oracle and the diffusion operator both reduce to a
// multi-controlled-Z, which we build from Toffoli (CCX) gates with a small
// ladder of ancilla qubits.
//
// Raise `n` to push ket harder: the state vector grows as 2^(n + ancillas) and
// the iteration count as sqrt(2^n).
#include <algorithm>
#include <cmath>
#include <complex>
#include <cstddef>
#include <iostream>
#include <utility>
#include <vector>

#include <ket/ket.hpp>

namespace {

// Multi-controlled X (generalized Toffoli): flips `target` iff every control is
// 1. Built from CCX with a compute/uncompute ladder over `anc` (needs
// controls-2 ancillas, all left back in |0>).
void mcx(ket::Circuit& c, const std::vector<std::size_t>& ctrl,
         std::size_t target, const std::vector<std::size_t>& anc) {
  const std::size_t k = ctrl.size();
  if (k == 0) {
    c.x(target);
    return;
  }
  if (k == 1) {
    c.cx(ctrl[0], target);
    return;
  }
  if (k == 2) {
    c.ccx(ctrl[0], ctrl[1], target);
    return;
  }
  c.ccx(ctrl[0], ctrl[1], anc[0]);
  for (std::size_t i = 2; i + 1 < k; ++i)
    c.ccx(anc[i - 2], ctrl[i], anc[i - 1]);
  c.ccx(anc[k - 3], ctrl[k - 1], target);
  for (std::size_t i = k - 2; i >= 2; --i)
    c.ccx(anc[i - 2], ctrl[i], anc[i - 1]);
  c.ccx(ctrl[0], ctrl[1], anc[0]);
}

// Multi-controlled Z on `qs` (a phase flip on the all-ones state), via
// H * MCX * H on the last qubit.
void mcz(ket::Circuit& c, const std::vector<std::size_t>& qs,
         const std::vector<std::size_t>& anc) {
  if (qs.size() == 1) {
    c.z(qs[0]);
    return;
  }
  const std::vector<std::size_t> ctrl(qs.begin(), qs.end() - 1);
  const std::size_t target = qs.back();
  c.h(target);
  mcx(c, ctrl, target, anc);
  c.h(target);
}

// Phase-flip |marked> (the oracle), by mapping it to the all-ones state, doing
// a multi-controlled Z, and mapping back.
void oracle(ket::Circuit& c, const std::vector<std::size_t>& search,
            std::size_t marked, const std::vector<std::size_t>& anc) {
  for (std::size_t i = 0; i < search.size(); ++i)
    if (((marked >> i) & 1u) == 0) c.x(search[i]);
  mcz(c, search, anc);
  for (std::size_t i = 0; i < search.size(); ++i)
    if (((marked >> i) & 1u) == 0) c.x(search[i]);
}

// Diffusion (inversion about the mean): reflect about the uniform state.
void diffusion(ket::Circuit& c, const std::vector<std::size_t>& search,
               const std::vector<std::size_t>& anc) {
  for (std::size_t q : search) c.h(q);
  for (std::size_t q : search) c.x(q);
  mcz(c, search, anc);
  for (std::size_t q : search) c.x(q);
  for (std::size_t q : search) c.h(q);
}

void print_bits(std::size_t value, std::size_t n) {
  for (std::size_t i = n; i-- > 0;) std::cout << ((value >> i) & 1u);
}

}  // namespace

int main() {
  const std::size_t n = 10;  // search qubits — raise to stress ket
  const std::size_t marked =
      std::size_t{0b10110101} & ((std::size_t{1} << n) - 1);  // item to find
  const std::size_t n_anc = (n >= 3) ? (n - 3) : 0;  // ancillas for the ladder
  const std::size_t total = n + n_anc;

  std::vector<std::size_t> search(n);
  for (std::size_t i = 0; i < n; ++i) search[i] = i;
  std::vector<std::size_t> anc(n_anc);
  for (std::size_t j = 0; j < n_anc; ++j) anc[j] = n + j;

  ket::Circuit c{total};

  // Uniform superposition over the search register.
  for (std::size_t q : search) c.h(q);

  const double pi = std::acos(-1.0);
  const std::size_t bign = std::size_t{1} << n;
  const int iters = std::max(
      1, static_cast<int>(
             std::floor(pi / 4.0 * std::sqrt(static_cast<double>(bign)))));

  for (int it = 0; it < iters; ++it) {
    oracle(c, search, marked, anc);
    diffusion(c, search, anc);
  }

  std::cout << "Grover's search\n"
            << "  search qubits : " << n << "  (database size " << bign << ")\n"
            << "  ancilla qubits: " << n_anc << "  (total " << total << ")\n"
            << "  marked item   : ";
  print_bits(marked, n);
  std::cout << "  (= " << marked << ")\n"
            << "  iterations    : " << iters << "\n";

  // Simulate. The ancillas return to |0>, so the search register's amplitude
  // for value v sits at state index v. (Large registers take a while.)
  std::cout << "  simulating " << total << " qubits (2^" << total
            << " amplitudes)..." << std::flush;
  const ket::State state = ket::run(c);
  std::cout << " done\n"
            << "  P(marked)     : " << std::norm(state[marked]) << "\n\n";

  std::vector<std::pair<double, std::size_t>> probs;
  probs.reserve(bign);
  for (std::size_t v = 0; v < bign; ++v)
    probs.emplace_back(std::norm(state[v]), v);
  std::sort(probs.rbegin(), probs.rend());

  std::cout << "Top outcomes:\n";
  for (std::size_t r = 0; r < std::min<std::size_t>(5, probs.size()); ++r) {
    std::cout << "  ";
    print_bits(probs[r].second, n);
    std::cout << "  p=" << probs[r].first << "\n";
  }

  std::cout << "\nMost likely outcome "
            << (probs[0].second == marked ? "matches" : "does NOT match")
            << " the marked item.\n";
  return 0;
}
