# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
"""Python-side tests for the Ket bindings, mirroring the C++ suite."""

import math

import ket
import pytest

INV_SQRT2 = 1.0 / math.sqrt(2.0)


def test_circuit_basics():
    c = ket.Circuit(2)
    assert c.n_qubits == 2
    c.h(0)
    c.cx(0, 1)
    assert "Circuit" in repr(c)


def test_bell_state_amplitudes():
    c = ket.Circuit(2)
    c.h(0)
    c.cx(0, 1)
    state = ket.run(c)

    assert len(state) == 4
    assert state[0] == pytest.approx(complex(INV_SQRT2, 0.0))
    assert state[1] == pytest.approx(0.0)
    assert state[2] == pytest.approx(0.0)
    assert state[3] == pytest.approx(complex(INV_SQRT2, 0.0))


def test_statevector_print():
    c = ket.Circuit(2)
    c.h(0)
    c.cx(0, 1)
    state = ket.run(c)
    assert state.print() == ("|00⟩: 0.707107\n|01⟩: 0\n|10⟩: 0\n|11⟩: 0.707107\n")
    assert str(state) == state.print()


def test_circuit_print():
    c = ket.Circuit(2)
    c.h(0)
    c.cx(0, 1)
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
    c.cx(0, 1)
    state = ket.run(c)
    a = [ket.measure(state, seed=42) for _ in range(5)]
    b = [ket.measure(state, seed=42) for _ in range(5)]
    assert a == b


def test_measure_bell_only_correlated_outcomes():
    c = ket.Circuit(2)
    c.h(0)
    c.cx(0, 1)
    state = ket.run(c)
    outcomes = {ket.measure(state, seed=s) for s in range(200)}
    # Bell state collapses only to |00> (0) or |11> (3).
    assert outcomes == {0, 3}


def test_measure_without_seed_is_valid():
    state = ket.run(ket.Circuit(1))  # ground state |0>
    for _ in range(50):
        assert ket.measure(state) == 0


def test_rotation_gates():
    import math

    c = ket.Circuit(1)
    c.ry(0, math.pi / 2)  # |0> -> equal superposition
    s = ket.run(c)
    assert abs(s[0]) == pytest.approx(INV_SQRT2)
    assert abs(s[1]) == pytest.approx(INV_SQRT2)
    assert "Ry(" in c.print()

    # Rz only changes phase: computational-basis probabilities are unchanged.
    c2 = ket.Circuit(1)
    c2.h(0)
    c2.rz(0, math.pi / 2)
    s2 = ket.run(c2)
    assert abs(s2[0]) ** 2 == pytest.approx(0.5)
    assert abs(s2[1]) ** 2 == pytest.approx(0.5)


def test_s_and_t_gates():
    import math

    # S|1> = i|1>; T twice == S.
    c = ket.Circuit(1)
    c.x(0)
    c.s(0)
    assert ket.run(c)[1] == pytest.approx(complex(0.0, 1.0))

    c2 = ket.Circuit(1)
    c2.x(0)
    c2.t(0)
    r = 1.0 / math.sqrt(2.0)
    assert ket.run(c2)[1] == pytest.approx(complex(r, r))

    # S-dagger renders with the dagger glyph.
    c3 = ket.Circuit(1)
    c3.sdg(0)
    assert "S†" in c3.print()


def test_pauli_y():
    c = ket.Circuit(1)
    c.y(0)  # Y|0> = i|1>
    s = ket.run(c)
    assert s[0] == pytest.approx(complex(0.0, 0.0))
    assert s[1] == pytest.approx(complex(0.0, 1.0))


def test_controlled_phase_gates():
    import math

    # CZ puts a -1 on |11>.
    c = ket.Circuit(2)
    c.x(0)
    c.x(1)
    c.cz(0, 1)
    s = ket.run(c)
    assert s[3] == pytest.approx(complex(-1.0, 0.0))

    # CP(pi/2) puts an i on |11>, and renders as a P(...) box.
    c2 = ket.Circuit(2)
    c2.x(0)
    c2.x(1)
    c2.cp(0, 1, math.pi / 2)
    s2 = ket.run(c2)
    assert s2[3] == pytest.approx(complex(0.0, 1.0))
    assert "P(" in c2.print()


def test_probe_capture_and_render():
    c = ket.Circuit(2)
    c.h(0)
    c.probe("a")
    c.cx(0, 1)
    c.probe("b")

    drawing = c.print()
    assert "↑" in drawing
    assert "a" in drawing and "b" in drawing

    r = ket.run_with_probes(c)
    assert [label for label, _ in r.probes] == ["a", "b"]
    # ψ_a after H: (|00> + |01>)/sqrt(2)
    s_a = r.probes[0][1]
    assert s_a[0] == pytest.approx(complex(INV_SQRT2, 0.0))
    assert s_a[1] == pytest.approx(complex(INV_SQRT2, 0.0))
    # final is the Bell state
    assert r.final[3] == pytest.approx(complex(INV_SQRT2, 0.0))


def test_ch_gate():
    c = ket.Circuit(2)
    c.x(0)  # |01>
    c.ch(0, 1)  # control=1 -> H on target -> (|01> + |11>)/sqrt(2)
    s = ket.run(c)
    assert s[1] == pytest.approx(complex(INV_SQRT2, 0.0))
    assert s[3] == pytest.approx(complex(INV_SQRT2, 0.0))


def test_controlled_rotation_gates():
    import math

    # crx(pi) with control set acts like rx(pi) on the target: |01> -> -i|11>.
    c = ket.Circuit(2)
    c.x(0)  # |01>
    c.crx(0, 1, math.pi)
    assert ket.run(c)[3] == pytest.approx(complex(0.0, -1.0))

    # cry(pi/2) makes an equal real superposition on the target.
    c2 = ket.Circuit(2)
    c2.x(0)
    c2.cry(0, 1, math.pi / 2)
    s2 = ket.run(c2)
    assert s2[1] == pytest.approx(complex(INV_SQRT2, 0.0))
    assert s2[3] == pytest.approx(complex(INV_SQRT2, 0.0))

    # crz only adds a relative phase; it renders as an Rz(...) box.
    c3 = ket.Circuit(2)
    c3.crz(0, 1, math.pi / 2)
    assert "Rz(" in c3.print()


def test_cu_gate():
    import math

    # cu(pi/2, 0, pi) is controlled-H: |01> -> (|01> + |11>)/sqrt(2).
    c = ket.Circuit(2)
    c.x(0)  # |01>
    c.cu(0, 1, math.pi / 2, 0, math.pi)
    s = ket.run(c)
    assert s[1] == pytest.approx(complex(INV_SQRT2, 0.0))
    assert s[3] == pytest.approx(complex(INV_SQRT2, 0.0))

    # Renders as a controlled U(...) box and serializes as cu3 in QASM.
    assert "U(" in c.print()
    assert "cu3(" in ket.to_qasm(c)


def test_cy_gate():
    c = ket.Circuit(2)
    c.x(0)  # |01>
    c.cy(0, 1)  # control=1 -> Y on target -> i|11>
    s = ket.run(c)
    assert s[3] == pytest.approx(complex(0.0, 1.0))


def test_ccx_gate():
    # Both controls set -> target flips: |011> -> |111> (index 7).
    c = ket.Circuit(3)
    c.x(0)
    c.x(1)
    c.ccx(0, 1, 2)
    s = ket.run(c)
    assert s[7] == pytest.approx(complex(1.0, 0.0))

    # Only one control set -> no flip; renders with two control dots.
    c2 = ket.Circuit(3)
    c2.x(0)
    c2.ccx(0, 1, 2)
    assert ket.run(c2)[1] == pytest.approx(complex(1.0, 0.0))
    assert c2.print().count("■") == 2


def test_cswap_gate():
    # Control set -> the two targets swap: |011> -> |101> (index 5).
    c = ket.Circuit(3)
    c.x(0)  # control
    c.x(1)  # |011>
    c.cswap(0, 1, 2)
    s = ket.run(c)
    assert s[5] == pytest.approx(complex(1.0, 0.0))

    # Control off -> no swap; renders with a control dot and two × marks.
    c2 = ket.Circuit(3)
    c2.x(1)  # |010>
    c2.cswap(0, 1, 2)
    assert ket.run(c2)[2] == pytest.approx(complex(1.0, 0.0))
    drawing = c2.print()
    assert "■" in drawing
    assert drawing.count("╳") == 2


def test_swap_gate():
    c = ket.Circuit(2)
    c.x(0)  # |01>
    c.swap(0, 1)  # -> |10>
    s = ket.run(c)
    assert s[2] == pytest.approx(complex(1.0, 0.0))
    assert "╳" in c.print()


def test_u_gate():
    import math

    c = ket.Circuit(1)
    c.u(0, math.pi / 2, 0, math.pi)  # U(pi/2, 0, pi) == H
    s = ket.run(c)
    assert abs(s[0]) == pytest.approx(INV_SQRT2)
    assert abs(s[1]) == pytest.approx(INV_SQRT2)

    # u3 from QASM maps to U; round-trips through to_qasm.
    c2 = ket.from_qasm("qreg q[1]; u3(pi/2,0,pi) q[0];")
    assert "U(" in ket.to_qasm(c2)


def test_qasm_parse_and_serialize():
    c = ket.from_qasm("OPENQASM 2.0;\nqreg q[2];\nh q[0];\ncx q[0],q[1];\n")
    assert c.n_qubits == 2
    s = ket.run(c)
    assert s[0] == pytest.approx(complex(INV_SQRT2, 0.0))
    assert s[3] == pytest.approx(complex(INV_SQRT2, 0.0))

    qasm = ket.to_qasm(c)
    assert "cx q[0],q[1];" in qasm


def test_measure_and_sample():
    c = ket.Circuit(2)
    c.h(0)
    c.cx(0, 1)
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
    bell.cx(0, 1)

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
    c.barrier("sync")  # all qubits, labeled
    c.cx(0, 1)
    c.barrier([1])  # subset

    drawing = c.print()
    assert "░" in drawing
    assert "sync" in drawing

    # The barrier must not change the simulated state (still a Bell state).
    state = ket.run(c)
    assert state[0] == pytest.approx(complex(INV_SQRT2, 0.0))
    assert state[3] == pytest.approx(complex(INV_SQRT2, 0.0))
    assert state[1] == pytest.approx(0.0)
    assert state[2] == pytest.approx(0.0)
