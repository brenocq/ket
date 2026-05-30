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


def test_measure_and_sample():
    c = ket.Circuit(2)
    c.h(0)
    c.cnot(0, 1)
    c.measure_all()

    assert c.n_clbits == 2
    drawing = c.print()
    assert "M" in drawing
    assert "c: 2/" in drawing

    # run() ignores measurements -> still the Bell state.
    state = ket.run(c)
    assert state[0] == pytest.approx(complex(INV_SQRT2, 0.0))
    assert state[3] == pytest.approx(complex(INV_SQRT2, 0.0))

    # Each shot fills the classical register; Bell is perfectly correlated.
    for seed in range(20):
        creg = ket.sample(c, seed=seed)
        assert len(creg) == 2
        assert creg[0] == creg[1]


def test_sample_size_matches_clbits_not_qubits():
    c = ket.Circuit(3)
    c.x(2)
    c.measure(2, 0)  # one qubit -> one clbit
    assert c.n_clbits == 1
    creg = ket.sample(c, seed=0)
    assert creg == [1]


def test_composite_block_append_decompose_and_simulate():
    bell = ket.Circuit(2, "bell")
    bell.h(0)
    bell.cnot(0, 1)

    c = ket.Circuit(2)
    c.append(bell, [0, 1])

    # Renders as a single labeled box.
    drawing = c.print()
    assert "bell" in drawing
    assert "┤0" in drawing

    # Simulating the block matches an inlined Bell state.
    state = ket.run(c)
    assert state[0] == pytest.approx(complex(INV_SQRT2, 0.0))
    assert state[3] == pytest.approx(complex(INV_SQRT2, 0.0))

    # decompose() expands the block into primitive gates.
    expanded = c.decompose()
    assert "bell" not in expanded.print()
    assert "H" in expanded.print()


def test_barrier_renders_and_is_noop():
    c = ket.Circuit(2)
    c.h(0)
    c.barrier("sync")       # all qubits, labeled
    c.cnot(0, 1)
    c.barrier([1])          # subset

    drawing = c.print()
    assert "░" in drawing
    assert "sync" in drawing

    # The barrier must not change the simulated state (still a Bell state).
    state = ket.run(c)
    assert state[0] == pytest.approx(complex(INV_SQRT2, 0.0))
    assert state[3] == pytest.approx(complex(INV_SQRT2, 0.0))
    assert state[1] == pytest.approx(0.0)
    assert state[2] == pytest.approx(0.0)
