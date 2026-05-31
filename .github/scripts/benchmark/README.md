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
`--reps` to change the run count.

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

The timed task is **evolve the circuit and draw one measurement sample**, and
each library uses its **best method for that circuit** — a stabilizer engine for
Clifford circuits, a state vector otherwise — chosen automatically where the
library supports it (Ket's `auto`, Aer's `automatic`) or selected by the harness
otherwise (Cirq's `CliffordSimulator`, PennyLane's `default.clifford`). Only the
sampling call is timed (parsing and setup are excluded), reported as the
**median** over several runs after a warm-up, on the same thread count.

The circuits ([`circuits.py`](circuits.py)) cover four regimes:

- **Bell (2q)** — per-call overhead, not the kernel.
- **Dense (16-qubit QFT and random)** — every library pays the `2ⁿ` cost, so
  this is state-vector throughput.
- **Deep dense (22-qubit)** — bandwidth-bound, so this is where **gate fusion**
  pays off: a fusing simulator (Aer) collapses several gates into one pass over
  the amplitudes and pulls ahead of ket's one-pass-per-gate kernel. Cirq (pure
  Python) and Quantum++ (no fusion) take minutes per shot here, so they're
  skipped.
- **Clifford (16- and 64-qubit)** — a stabilizer simulator runs these in `O(n²)`.
  The 64-qubit case is out of reach for any state vector, so libraries without a
  stabilizer engine (Quantum++) are skipped — exactly the point of the
  circuit-specific optimization.

A library declares which `(n, clifford)` circuits it can handle via `supports()`,
and each circuit may name libraries to `skip`, so the harness leaves out cells a
library can't reach (or would be hopelessly slow on) instead of crashing.

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

| Library | Dense backend | Clifford backend | Requirement |
| --- | --- | --- | --- |
| Ket | C++ state vector | C++ stabilizer (`auto`) | `pip install .` |
| Qiskit (Aer) | `AerSimulator` | stabilizer (`automatic`) | `pip install qiskit qiskit-aer` |
| Cirq | `cirq.Simulator` | `cirq.CliffordSimulator` | `pip install cirq-core ply` |
| PennyLane | `lightning.qubit` | `default.clifford` (stim) | `pip install pennylane pennylane-lightning pennylane-qiskit stim` |
| Quantum++ | C++ state vector | — (none) | CMake + a C++ compiler (auto-built) |

Quantum++ has no Python API, so `adapters/qpp_adapter.py` builds a small C++
harness (`qpp_bench/`) that loads the QASM and times the simulation itself (and
can print its state vector for the correctness check); it is fetched and compiled
on the first run.

## Extending

- **Add a circuit:** add an entry to `timing_circuits()` in `benchmark.py` — a
  generator from `circuits.py` or any QASM string, plus its `clifford` flag. It
  becomes a new group.
- **Add a library:** create `adapters/<name>_adapter.py` with a subclass of
  `Adapter` (usually `PythonAdapter`, implementing `load` and `sample_once`,
  plus `supports` and `statevector` for the correctness check). It is discovered
  automatically and included whenever `available()` is true.
