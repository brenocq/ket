# Simulator benchmark

Checks that ket's state-vector simulator agrees with other quantum libraries,
then times them against each other on a set of OpenQASM circuits and writes a
grouped bar chart (one group per circuit).

```sh
pip install .                                  # install ket (from the repo root)
pip install -r .github/scripts/benchmark/requirements.txt
python .github/scripts/benchmark/benchmark.py             # single-threaded
python .github/scripts/benchmark/benchmark.py --multicore # 8 threads per backend
```

A library that is not installed is skipped, so it shows whatever you have. Use
`--reps` / `--big-reps` to change run counts.

## Single-core vs. multi-core

There are two charts because they answer different questions:

| command | threads | output | what it shows |
| --- | --- | --- | --- |
| `benchmark.py` | 1 | `results/benchmark.png` | the bare kernel, head to head |
| `benchmark.py --multicore` | 8 | `results/benchmark-8core.png` | how the libraries actually ship |
| `benchmark.py --threads N` | N | `results/benchmark-Ncore.png` | any thread budget |

The multi-core run pins every OpenMP/BLAS backend (Aer, lightning, qpp) to the
chosen thread count. **Ket has no threading, so it is identical in both** — which
is exactly the gap the multi-core chart is meant to show.

## What it measures

For each circuit and library, only the **simulation** is timed (parsing and
circuit construction are excluded), reported as the **median** over several runs
after a warm-up, with the standard deviation as an error bar. Every library
computes the full **state vector**, on the same thread count.

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

- **Threads:** the default single-threaded run compares the kernel, not core
  count; `--multicore` enables the multi-threading Aer/lightning/qpp ship with.
  Either way ket is single-core (it has no threading), so on `--multicore` the
  others pull further ahead — the honest picture.
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
