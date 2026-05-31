# Simulator benchmark

Checks that ket's state-vector simulator agrees with other quantum libraries,
then times them against each other on a set of OpenQASM circuits and writes a
grouped bar chart (one group per circuit).

```sh
pip install .                                  # install ket (from the repo root)
pip install -r .github/scripts/benchmark/requirements.txt
python .github/scripts/benchmark/benchmark.py  # --reps / --big-reps to change run counts
```

The chart lands in `results/benchmark.png`. A library that is not installed is
skipped, so it shows whatever you have.

## What it measures

For each circuit and library, only the **simulation** is timed (parsing and
circuit construction are excluded), reported as the **median** over several runs
after a warm-up, with the standard deviation as an error bar. Every library runs
**single-threaded** (`OMP_NUM_THREADS=1`) and computes the full **state vector**.

Two circuit sizes matter:

- **Small** circuits (Bell) are dominated by per-call overhead — Python
  dispatch, object construction, job setup — not the kernel.
- **Large** circuits (24-qubit GHZ, QFT, and a random brickwork circuit, in
  [`circuits.py`](circuits.py)) put the `2ⁿ` amplitude update in charge, which is
  what a simulator's throughput actually is.

## Correctness

Before timing, every library simulates a few small circuits and its final state
vector is compared against ket's. The check is up to **global phase** and
**qubit-ordering convention** (libraries split between little- and big-endian),
so it compares `|⟨ref|candidate⟩|` and also tries the bit-reversed vector. A
mismatch is flagged loudly — a fast wrong answer is worthless. Correctness is a
property of the kernel, not the width, so small instances are enough.

## Fairness caveats

- **Single-threaded** is deliberate (compare the algorithm, not core count), but
  it disables the multi-threading that Aer and lightning are built around.
- **Build flags:** ket and Quantum++ are compiled locally (so they pick up the
  host's instruction set), while the comparison libraries use generic **pip
  wheels** built for broad CPU compatibility. The chart records each library's
  version, and notes this in its footnote.

## Libraries

| Library | Backend | Requirement |
| --- | --- | --- |
| ket | our C++ state vector | `pip install .` |
| Qiskit (Aer) | C++ `AerSimulator` | `pip install qiskit qiskit-aer` |
| Cirq | Python `cirq.Simulator` | `pip install cirq-core ply` |
| PennyLane (lightning) | C++ `lightning.qubit` | `pip install pennylane pennylane-lightning pennylane-qiskit` |
| Quantum++ | C++ header-only | CMake + a C++ compiler (auto-built) |

Quantum++ has no Python API, so `adapters/qpp_adapter.py` builds a small C++
harness (`qpp_bench/`) that loads the QASM and times the simulation itself (and
can print its state vector for the correctness check); it is fetched and compiled
on the first run.

## Extending

- **Add a circuit:** add an entry to `timing_circuits()` in `benchmark.py` — a
  generator from `circuits.py` or any QASM string. It becomes a new group.
- **Add a library:** create `adapters/<name>_adapter.py` with a subclass of
  `Adapter` (usually `PythonAdapter`, implementing `load`, `simulate`, and —
  for the correctness check — `state`). It is discovered automatically and
  included whenever `available()` is true.
