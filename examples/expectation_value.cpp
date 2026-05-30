// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
//
// Expectation values of observables: <psi|P|psi> for a Pauli string P, and for
// a Hamiltonian (a coefficient-weighted sum of Pauli strings). Expectation
// values turn a quantum state into the real numbers algorithms actually
// optimize. Here we (1) read off the correlations of a Bell state, and (2) run
// a tiny VQE that minimizes the energy <H> of a one-qubit Hamiltonian.
#include <cmath>
#include <iostream>

#include <ket/ket.hpp>

int main() {
  // 1) Bell state correlations, from single Pauli strings.
  ket::Circuit bell{2};
  bell.h(0);
  bell.cx(0, 1);
  const ket::State psi = ket::run(bell);

  std::cout << "Bell state (|00> + |11>)/sqrt(2):\n"
            << "  <ZZ> = " << ket::expval(psi, "ZZ") << "\n"
            << "  <XX> = " << ket::expval(psi, "XX") << "\n"
            << "  <YY> = " << ket::expval(psi, "YY") << "\n"
            << "  <ZI> = " << ket::expval(psi, "ZI") << "   (qubit 1 alone)\n"
            << "  <IZ> = " << ket::expval(psi, "IZ") << "   (qubit 0 alone)\n"
            << "  -> each qubit looks random, yet they are perfectly "
               "correlated.\n\n";

  // 2) Tiny VQE: minimize the energy <H> of H = 0.5 Z + 0.5 X over a one-qubit
  //    Ry(theta) ansatz. The exact ground-state energy is -sqrt(0.5).
  const ket::PauliSum hamiltonian = {{0.5, "Z"}, {0.5, "X"}};

  const double pi = std::acos(-1.0);
  double best_theta = 0.0;
  double best_energy = 2.0;  // any real energy is below this
  for (int k = 0; k <= 360; ++k) {
    const double theta = 2.0 * pi * static_cast<double>(k) / 360.0;
    ket::Circuit ansatz{1};
    ansatz.ry(0, theta);
    const double energy = ket::expval(ket::run(ansatz), hamiltonian);
    if (energy < best_energy) {
      best_energy = energy;
      best_theta = theta;
    }
  }

  std::cout << "VQE for H = 0.5 Z + 0.5 X  (1 qubit, Ry ansatz):\n"
            << "  best theta   = " << best_theta << "\n"
            << "  min <H>      = " << best_energy << "\n"
            << "  exact ground = " << -std::sqrt(0.5) << "\n";
  return 0;
}
