#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
"""Benchmark ket's state-vector simulator against other libraries.

Runs each OpenQASM circuit in every available library, times the simulation
(averaged over many runs), and writes a grouped bar chart to results/.

  python benchmark.py [--reps N]

Libraries that are not installed (or, for Quantum++, cannot be built) are
skipped. To keep the comparison about the simulation algorithm rather than
core count, everything runs single-threaded.
"""

import argparse
import importlib
import os
import sys

# Pin every OpenMP-based backend (Aer, lightning, qpp, ...) to one thread for an
# apples-to-apples comparison. Must happen before those libraries are imported.
os.environ.setdefault("OMP_NUM_THREADS", "1")

HERE = os.path.dirname(os.path.abspath(__file__))
ADAPTERS_DIR = os.path.join(HERE, "adapters")
REPO_ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
sys.path.insert(0, ADAPTERS_DIR)

# Circuit name -> QASM file. Add an entry to benchmark another circuit.
CIRCUITS = {
    "Bell": os.path.join(REPO_ROOT, "examples", "bell.qasm"),
    "Grover": os.path.join(REPO_ROOT, "examples", "grover.qasm"),
}

# Left-to-right order of the bars within each group. Adapters whose name starts
# with one of these prefixes are sorted accordingly; any others follow, in
# discovery order.
PLOT_ORDER = ["Ket", "Quantum++", "Qiskit", "Cirq", "PennyLane"]


def plot_order_key(name):
    for i, prefix in enumerate(PLOT_ORDER):
        if name.startswith(prefix):
            return i
    return len(PLOT_ORDER)


def clean_qasm(text: str) -> str:
    """Strip measurements/classical registers/barriers so every library runs
    the same pure unitary and reports a state vector."""
    keep = []
    for line in text.splitlines():
        head = line.strip().split(" ", 1)[0]
        if head in ("measure", "creg", "barrier"):
            continue
        keep.append(line)
    return "\n".join(keep) + "\n"


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


def run(reps: int):
    print(f"Discovering simulators (single-threaded, {reps} runs each):")
    available = []
    for adapter in discover_adapters():
        if adapter.available():
            available.append(adapter)
            print(f"  [ok]      {adapter.name}")
        else:
            print(f"  [skipped] {adapter.name}")

    results = {}  # results[circuit][adapter_name] = avg seconds
    for cname, path in CIRCUITS.items():
        qasm = clean_qasm(open(path, encoding="utf-8").read())
        print(f"\n{cname}  ({path}):")
        for adapter in available:
            try:
                seconds = adapter.benchmark(qasm, reps)
                results.setdefault(cname, {})[adapter.name] = seconds
                print(f"  {adapter.name:24s} {seconds * 1e3:10.4f} ms")
            except Exception as exc:  # noqa: BLE001 - keep going on a bad backend
                print(f"  {adapter.name:24s} FAILED: {exc}")
    return [a.name for a in available], results


def plot(adapter_names, results, reps, out_path):
    import matplotlib

    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    import numpy as np

    adapter_names = sorted(adapter_names, key=plot_order_key)
    circuits = list(results.keys())
    x = np.arange(len(circuits))
    width = 0.8 / max(1, len(adapter_names))

    fig, ax = plt.subplots(figsize=(8, 5))
    for i, name in enumerate(adapter_names):
        heights = [results[c].get(name, np.nan) * 1e3 for c in circuits]  # ms
        offset = (i - (len(adapter_names) - 1) / 2) * width
        bars = ax.bar(x + offset, heights, width, label=name)
        ax.bar_label(bars, fmt="%.3g", fontsize=7, padding=2)

    ax.set_yscale("log")
    ax.set_ylabel("simulation time (ms) — lower is better")
    ax.set_xticks(x)
    ax.set_xticklabels(circuits)
    ax.set_title(
        f"OpenQASM state-vector simulation\n(single-threaded, average of {reps} runs)"
    )
    ax.legend()
    ax.grid(axis="y", which="both", linewidth=0.3, alpha=0.4)

    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    fig.tight_layout()
    fig.savefig(out_path, dpi=150)
    print(f"\nWrote {out_path}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reps", type=int, default=50, help="runs per measurement")
    parser.add_argument(
        "--out",
        default=os.path.join(HERE, "results", "benchmark.png"),
        help="output PNG path",
    )
    args = parser.parse_args()

    adapter_names, results = run(args.reps)
    if results:
        plot(adapter_names, results, args.reps, args.out)
    else:
        print("No results to plot.")


if __name__ == "__main__":
    main()
