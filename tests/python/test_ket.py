# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
"""Python-side tests for the Ket bindings, mirroring the C++ suite."""

import math

import pytest

import ket

INV_SQRT2 = 1.0 / math.sqrt(2.0)


def test_circuit_basics():
    c = ket.Circuit(2)
    assert c.n_qubits == 2
    c.h(0)
    c.cnot(0, 1)
    assert "Circuit" in repr(c)


def test_bell_state_amplitudes():
    c = ket.Circuit(2)
    c.h(0)
    c.cnot(0, 1)
    state = ket.run(c)

    assert len(state) == 4
    assert state[0] == pytest.approx(complex(INV_SQRT2, 0.0))
    assert state[1] == pytest.approx(0.0)
    assert state[2] == pytest.approx(0.0)
    assert state[3] == pytest.approx(complex(INV_SQRT2, 0.0))


def test_statevector_print():
    c = ket.Circuit(2)
    c.h(0)
    c.cnot(0, 1)
    state = ket.run(c)
    assert state.print() == (
        "|00⟩: 0.707107\n"
        "|01⟩: 0\n"
        "|10⟩: 0\n"
        "|11⟩: 0.707107\n"
    )
    assert str(state) == state.print()


def test_circuit_print():
    c = ket.Circuit(2)
    c.h(0)
    c.cnot(0, 1)
    assert c.print() == (
        "     ┌───┐     \n"
        "q_0: ┤ H ├──■──\n"
        "     └───┘┌─┴─┐\n"
        "q_1: ─────┤ X ├\n"
        "          └───┘\n"
    )


def test_index_out_of_range():
    state = ket.run(ket.Circuit(1))
    with pytest.raises(IndexError):
        _ = state[99]


def test_measure_is_reproducible_with_seed():
    c = ket.Circuit(2)
    c.h(0)
    c.cnot(0, 1)
    state = ket.run(c)
    a = [ket.measure(state, seed=42) for _ in range(5)]
    b = [ket.measure(state, seed=42) for _ in range(5)]
    assert a == b


def test_measure_bell_only_correlated_outcomes():
    c = ket.Circuit(2)
    c.h(0)
    c.cnot(0, 1)
    state = ket.run(c)
    outcomes = {ket.measure(state, seed=s) for s in range(200)}
    # Bell state collapses only to |00> (0) or |11> (3).
    assert outcomes == {0, 3}


def test_measure_without_seed_is_valid():
    state = ket.run(ket.Circuit(1))  # ground state |0>
    for _ in range(50):
        assert ket.measure(state) == 0
