# Simulator benchmark

Times ket's state-vector simulator against other quantum libraries on a set of
OpenQASM circuits, and writes a grouped bar chart (one group per circuit).

```sh
pip install .                                  # install ket (from the repo root)
pip install -r .github/scripts/benchmark/requirements.txt
python .github/scripts/benchmark/benchmark.py  # --reps N to change the run count
```

The chart lands in `results/benchmark.png`.

## What it measures

For each circuit and each library, only the **simulation** is timed (parsing and
circuit construction are excluded), averaged over `--reps` runs after a warm-up.
Every library runs **single-threaded** (`OMP_NUM_THREADS=1`) so the comparison is
about the algorithm, not core count, and computes the full **state vector** — the
QASM's measurements/`creg`/`barrier` are stripped so each library runs the same
pure unitary.

A library that is not installed is skipped, so the chart shows whatever you have.

## Libraries

| Library | Backend | Requirement |
| --- | --- | --- |
| ket | our C++ state vector | `pip install .` |
| Qiskit (Aer) | C++ `AerSimulator` | `pip install qiskit qiskit-aer` |
| Cirq | Python `cirq.Simulator` | `pip install cirq-core` |
| PennyLane (lightning) | C++ `lightning.qubit` | `pip install pennylane pennylane-lightning` |
| Quantum++ | C++ header-only | CMake + a C++ compiler (auto-built) |

Quantum++ has no Python API, so `adapters/qpp_adapter.py` builds a small C++
harness (`qpp_bench/`) that loads the QASM and times the simulation itself; it is
fetched and compiled on the first run.

## Extending

- **Add a circuit:** drop a `.qasm` file in `examples/` and add it to `CIRCUITS`
  in `benchmark.py`. It becomes a new group in the chart. Larger circuits (20+
  qubits) are where simulation throughput — rather than per-call overhead —
  dominates, so they make the most informative comparison.
- **Add a library:** create `adapters/<name>_adapter.py` with a subclass of
  `Adapter` (usually `PythonAdapter`, implementing `load` and `simulate`). It is
  discovered automatically and included whenever `available()` is true.
