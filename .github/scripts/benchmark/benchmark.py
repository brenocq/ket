#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
"""Benchmark Ket's state-vector simulator against other libraries.

Verifies every library computes the same state, then times each on a set of
OpenQASM circuits (median over many runs) and writes a grouped bar chart.

  python benchmark.py                 # single-threaded (kernel comparison)
  python benchmark.py --multicore     # 8 threads
  python benchmark.py --threads N     # N threads

The suite mixes a small overhead reference, dense 16-qubit circuits, a deep dense
22-qubit circuit (bandwidth-bound, where gate fusion pays off), and large
Clifford circuits — each library picks its best method per circuit. Libraries
that aren't installed (or can't reach a circuit) are skipped.
"""

import argparse
import importlib
import math
import os
import statistics
import sys


def _resolve_threads():
    """Thread count from the CLI, parsed before any numeric library is imported
    (so the env vars below take effect). --threads N wins; --multicore means 8."""
    argv = sys.argv
    for i, a in enumerate(argv):
        try:
            if a.startswith("--threads="):
                return max(1, int(a.split("=", 1)[1]))
            if a == "--threads" and i + 1 < len(argv):
                return max(1, int(argv[i + 1]))
        except ValueError:
            return 1
    return 8 if "--multicore" in argv else 1


# Every OpenMP/BLAS backend reads these at import time, so pin them up front.
THREADS = _resolve_threads()
for _var in (
    "OMP_NUM_THREADS",
    "OPENBLAS_NUM_THREADS",
    "MKL_NUM_THREADS",
    "NUMEXPR_NUM_THREADS",
    "VECLIB_MAXIMUM_THREADS",
):
    os.environ[_var] = str(THREADS)

HERE = os.path.dirname(os.path.abspath(__file__))
ADAPTERS_DIR = os.path.join(HERE, "adapters")
REPO_ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
sys.path.insert(0, HERE)
sys.path.insert(0, ADAPTERS_DIR)

import circuits  # noqa: E402  (after sys.path setup)
from base import num_qubits  # noqa: E402

# Left-to-right order of the bars within each group. Adapters whose name starts
# with one of these prefixes sort accordingly; any others follow, in discovery
# order.
PLOT_ORDER = ["Ket", "Qiskit", "Quantum++", "Cirq", "PennyLane"]


def plot_order_key(name):
    for i, prefix in enumerate(PLOT_ORDER):
        if name.startswith(prefix):
            return i
    return len(PLOT_ORDER)


# Libraries the deep-dense circuit skips: a pure-Python kernel (Cirq) and a state
# vector with no gate fusion (Quantum++) take minutes per shot at 22 qubits, and
# aren't the comparison that circuit is about (fusion on a compiled kernel).
_NO_FUSION = frozenset({"Cirq", "Quantum++"})


def timing_circuits():
    """(name, qasm, clifford, skip) for the timed benchmark.

    `clifford` flags circuits a stabilizer simulator can handle, so each library
    picks its best method; `skip` names libraries to leave out of that circuit.
    The dense 16-qubit circuits are state-vector throughput; the large Clifford
    circuit is out of reach for any state vector; the deep dense 22-qubit circuit
    is bandwidth-bound, where gate fusion (which ket lacks) pays off."""
    none = frozenset()
    return [
        ("Bell (2q)", circuits.bell(), True, none),
        ("QFT (16q)", circuits.qft(16), False, none),
        ("Random (16q)", circuits.random_circuit(16, depth=10, seed=7), False, none),
        (
            "Deep dense (22q)",
            circuits.random_circuit(22, depth=4, seed=104),
            False,
            _NO_FUSION,
        ),
        ("Clifford (16q)", circuits.random_clifford(16, depth=20, seed=3), True, none),
        ("Clifford (64q)", circuits.random_clifford(64, depth=64, seed=5), True, none),
    ]


# Small circuits for the correctness cross-check. Correctness is a property of
# the kernel, not the width, so small representative instances suffice — and a
# non-symmetric random circuit is the strongest discriminator.
CHECK_CIRCUITS = [
    ("Bell", circuits.bell()),
    ("QFT-5", circuits.qft(5)),
    ("Random-5", circuits.random_circuit(5, depth=6, seed=1)),
]


def discover_adapters():
    from base import Adapter

    adapters = []
    for fname in sorted(os.listdir(ADAPTERS_DIR)):
        if not fname.endswith("_adapter.py"):
            continue
        module = importlib.import_module(fname[:-3])
        for attr in vars(module).values():
            if (
                isinstance(attr, type)
                and issubclass(attr, Adapter)
                and attr.__module__ == module.__name__
            ):
                adapters.append(attr())
    return adapters


def _bit_reverse_perm(n):
    import numpy as np

    idx = np.arange(1 << n)
    rev = np.zeros_like(idx)
    for b in range(n):
        rev |= ((idx >> b) & 1) << (n - 1 - b)
    return rev


def _agree(ref, vec, tol):
    """Do two state vectors match up to global phase and qubit-ordering
    convention? Returns (ok, fidelity). Global phase drops out of |⟨ref|vec⟩|;
    differing endianness is absorbed by also trying the bit-reversed vector."""
    import numpy as np

    ref = np.asarray(ref, dtype=complex)
    vec = np.asarray(vec, dtype=complex)
    if ref.shape != vec.shape:
        return False, 0.0
    nr, nv = np.linalg.norm(ref), np.linalg.norm(vec)
    if nr == 0 or nv == 0:
        return False, 0.0
    n = int(round(math.log2(len(ref))))
    perm = _bit_reverse_perm(n)
    fidelity = max(abs(np.vdot(ref, vec)), abs(np.vdot(ref, vec[perm]))) / (nr * nv)
    return (1 - fidelity) <= tol, fidelity


def check_correctness(adapters, tol=1e-4):
    print("\nCorrectness (final state vector, up to global phase & qubit order):")
    all_ok = True
    for cname, qasm in CHECK_CIRCUITS:
        states = {}
        for a in adapters:
            try:
                sv = a.statevector(qasm)
            except Exception as exc:  # noqa: BLE001
                print(f"  {cname}: {a.name} state failed: {exc}")
                sv = None
            if sv is not None:
                states[a.name] = sv
        if len(states) < 2:
            continue
        ref_name = "Ket" if "Ket" in states else next(iter(states))
        print(f"  {cname}  (reference: {ref_name})")
        for a in adapters:
            if a.name == ref_name:
                continue
            if a.name not in states:
                print(f"    [n/a ]  {a.name:24s} no state vector exposed")
                continue
            ok, fid = _agree(states[ref_name], states[a.name], tol)
            all_ok &= ok
            print(f"    [{'ok  ' if ok else 'FAIL'}]  {a.name:24s} fidelity={fid:.6f}")
    if not all_ok:
        print("  WARNING: a simulator disagrees — timings below are not comparable.")
    return all_ok


def _threads_label():
    return "single-threaded" if THREADS == 1 else f"{THREADS} threads"


def run(reps):
    print(f"Discovering simulators ({_threads_label()}, {reps} reps each):")
    available, versions = [], {}
    for adapter in discover_adapters():
        if adapter.available():
            available.append(adapter)
            try:
                versions[adapter.name] = adapter.version()
            except Exception:  # noqa: BLE001
                versions[adapter.name] = "?"
            print(f"  [ok]      {adapter.name:24s} {versions[adapter.name]}")
        else:
            print(f"  [skipped] {adapter.name}")

    check_correctness(available)

    results = {}  # results[circuit][adapter] = {"median": s, "std": s}
    for cname, qasm, clifford, skip in timing_circuits():
        n = num_qubits(qasm)
        kind = "Clifford" if clifford else "dense"
        print(f"\n{cname}  ({kind}, best method per library):")
        for adapter in available:
            if adapter.name in skip:
                print(f"  {adapter.name:24s} [skipped: no gate fusion, too slow]")
                continue
            if not adapter.supports(n, clifford):
                print(f"  {adapter.name:24s} [skipped: no {n}-qubit {kind} method]")
                continue
            try:
                times = adapter.benchmark(qasm, reps, clifford)
                median = statistics.median(times)
                std = statistics.stdev(times) if len(times) > 1 else 0.0
                results.setdefault(cname, {})[adapter.name] = {
                    "median": median,
                    "std": std,
                }
                print(f"  {adapter.name:24s} {median * 1e3:12.4f} ms  (n={len(times)})")
            except Exception as exc:  # noqa: BLE001 - keep going on a bad backend
                print(f"  {adapter.name:24s} FAILED: {exc}")
    return [a.name for a in available], results, versions


def plot(adapter_names, results, versions, out_path):
    import matplotlib

    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    import numpy as np

    present = {name for cell in results.values() for name in cell}
    adapter_names = [
        n for n in sorted(adapter_names, key=plot_order_key) if n in present
    ]
    circuit_names = list(results.keys())
    x = np.arange(len(circuit_names))
    width = 0.8 / max(1, len(adapter_names))

    fig, ax = plt.subplots(figsize=(9, 5.5))
    for i, name in enumerate(adapter_names):
        cells = [results[c].get(name) for c in circuit_names]
        heights = [(c["median"] * 1e3 if c else np.nan) for c in cells]  # ms
        errs = [(c["std"] * 1e3 if c else 0.0) for c in cells]
        offset = (i - (len(adapter_names) - 1) / 2) * width
        bars = ax.bar(x + offset, heights, width, label=name, yerr=errs, capsize=2)
        ax.bar_label(bars, fmt="%.3g", fontsize=6, padding=2)

    ax.set_yscale("log")
    ax.set_ylabel("evolve + one sample (ms) — lower is better")
    ax.set_xticks(x)
    ax.set_xticklabels(circuit_names)
    ax.set_title(
        "OpenQASM simulation, best method per circuit\n"
        f"({_threads_label()}; Clifford circuits may use a stabilizer engine; "
        "bars = median)"
    )
    ax.legend()
    ax.grid(axis="y", which="both", linewidth=0.3, alpha=0.4)

    vers = "   ".join(f"{n} {versions.get(n, '?')}" for n in adapter_names)
    head = (
        "Best method per circuit (a stabilizer engine for Clifford circuits). "
        "Cirq and Quantum++ are omitted from the deep dense circuit (no gate "
        "fusion, minutes per shot)."
    )
    note = head + "\n" + vers
    fig.text(0.5, 0.005, note, ha="center", va="bottom", fontsize=6, color="0.35")

    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    fig.tight_layout(rect=(0, 0.06, 1, 1))
    fig.savefig(out_path, dpi=150)
    print(f"\nWrote {out_path}")


def main():
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("--reps", type=int, default=10, help="timed runs per cell")
    # --threads / --multicore are parsed in _resolve_threads() before imports;
    # declared here so they appear in --help and aren't rejected as unknown.
    parser.add_argument(
        "--threads", type=int, default=None, help="threads per backend (default 1)"
    )
    parser.add_argument(
        "--multicore", action="store_true", help="shorthand for --threads 8"
    )
    parser.add_argument(
        "--out", default=None, help="output PNG path (default depends on thread count)"
    )
    parser.add_argument(
        "--replot",
        action="store_true",
        help="skip simulation; redraw from the cached results JSON",
    )
    args = parser.parse_args()

    if args.out is None:
        name = "benchmark.png" if THREADS == 1 else f"benchmark-{THREADS}core.png"
        args.out = os.path.join(HERE, "results", name)

    import json

    data_path = os.path.splitext(args.out)[0] + ".json"

    if args.replot:
        with open(data_path, encoding="utf-8") as f:
            cached = json.load(f)
        plot(cached["adapter_names"], cached["results"], cached["versions"], args.out)
        return

    adapter_names, results, versions = run(args.reps)
    if not results:
        print("No results to plot.")
        return
    os.makedirs(os.path.dirname(data_path), exist_ok=True)
    with open(data_path, "w", encoding="utf-8") as f:
        json.dump(
            {"adapter_names": adapter_names, "results": results, "versions": versions},
            f,
            indent=2,
        )
    plot(adapter_names, results, versions, args.out)


if __name__ == "__main__":
    main()
