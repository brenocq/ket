<div align="center">
  <img width="360" height="300" alt="ket_title" src="https://github.com/user-attachments/assets/7e518df6-a526-45a4-8b9a-58f3a39b2833" />
</div>

**Ket** (|⟩) is a quantum computing library for C++20, with first-class Python
bindings. You build a circuit, simulate it on a built-in state-vector
simulator, and sample measurement outcomes — from C++, from Python, from the
command line, or in a small GUI.

Internally a circuit is a **directed acyclic graph (DAG)** of gates: each node is
a gate and edges connect gates that share a qubit. This makes gate dependencies
explicit and leaves room for future analysis and optimization passes.

## Features

- **A broad gate set** — Paulis, phase and `T` gates, parameterized rotations
  (`Rx`/`Ry`/`Rz`), the general single-qubit `U`, their controlled forms
  (including controlled rotations and controlled-phase), `SWAP`, and the
  three-qubit Toffoli (`CCX`) and Fredkin (`CSWAP`).
- **Exact state-vector simulation** of the full `2ⁿ` amplitude vector.
- **Measurement and sampling** into a classical register, for shot experiments.
- **Composite gates** — package a sub-circuit into a reusable, labeled block,
  and `decompose()` it back into primitives.
- **Qiskit-style ASCII diagrams** via `circuit.print()`.
- **OpenQASM 2.0** import and export, including user-defined `gate` blocks.
- **Python bindings** (built with [pybind11]) that mirror the C++ API.
- **Tooling** — a `ket-cli` command-line tool and a `ket-gui` circuit viewer.

The complete list of gate methods lives in
[`include/ket/circuit.hpp`](include/ket/circuit.hpp), and the
[`examples/`](examples) directory has runnable programs, from Bell and GHZ
states up to Grover's search.

## Quick start (C++)

```cpp
#include <ket/ket.hpp>
#include <iostream>

int main() {
  // A Bell state: H on q0, then CNOT(q0 -> q1).
  ket::Circuit c{2};
  c.h(0);
  c.cx(0, 1);

  std::cout << c.print();            // ASCII circuit diagram
  std::cout << ket::run(c).print();  // final state vector
}
```

```
     ┌───┐
q_0: ┤ H ├──■──
     └───┘┌─┴─┐
q_1: ─────┤ X ├
          └───┘
|00⟩: 0.707107
|01⟩: 0
|10⟩: 0
|11⟩: 0.707107
```

## Python

The Python API mirrors the C++ one — `Circuit`, `run`, `measure`, `sample` —
with a few Pythonic touches: `Circuit` and `State` render through
`print()`/`str()`, `State` supports `len()` and indexing (returning a Python
`complex`), and `measure`/`sample` take an optional `seed`.

The bindings compile from source via [scikit-build-core], so a C++20 compiler
and CMake are required:

```sh
python -m venv .venv
source .venv/bin/activate
pip install .                 # or: pip install -e ".[test]" for development
```

```python
import ket

c = ket.Circuit(2)
c.h(0)
c.cx(0, 1)
c.measure_all()

print(c)                       # circuit diagram
print(ket.run(c))              # state vector
print(ket.sample(c, seed=0))   # one shot, e.g. [0, 0] or [1, 1]
```

## Command line

`ket-cli` operates on OpenQASM files (or stdin, so it composes in pipelines):

```sh
ket-cli draw   examples/bell.qasm                 # ASCII diagram
ket-cli run    examples/bell.qasm                 # final state vector
ket-cli sample examples/bell.qasm --shots 1000    # measurement histogram
ket-cli --help
```

Build it with `./build.sh -c` (the binary lands at `build/cli/ket-cli`).

## GUI

`ket-gui` opens a circuit in a window: an editable QASM panel beside a
pan/zoom circuit view, built with GLFW, Dear ImGui, and ImPlot.

```sh
./build.sh -g
./build/gui/ket-gui examples/grover.qasm
```

## Building

Ket uses CMake and a C++20 compiler. The `build.sh` helper wraps the common
workflows (its flags map to the underlying CMake options); everything beyond
the core library is opt-in.

```sh
./build.sh          # build the C++ library
./build.sh -ct      # build and run the C++ tests
./build.sh -pt      # build the Python bindings and run their tests
./build.sh -e       # build the example programs
./build.sh -c       # build the ket-cli executable
./build.sh -g       # build the ket-gui executable
./build.sh --help   # all options
```

Tests use GoogleTest and the GUI's dependencies are fetched automatically; you
can also drive CMake directly (`cmake -S . -B build -DKET_TESTS=ON`, …). To use
Ket in your own project, link the `ket` target, add `include/` to your include
path, and include the umbrella header `<ket/ket.hpp>`.

## How it works

A register of *n* qubits is a vector of `2ⁿ` complex amplitudes. Each gate is
applied as an in-place linear transformation over that vector — qubit `i` is
bit `i` of the basis-state index (little-endian). The simulator walks the DAG
in topological order (which matches insertion order) and applies each gate in
turn. **Measurement** samples a basis state by the Born rule (outcome `i` with
probability `|amplitudeᵢ|²`), with an optional seed for reproducibility.

Because the state vector stores all `2ⁿ` amplitudes, simulation is exact but
bounded by memory — practical up to roughly 25 qubits. There is no noise
modeling.

## Roadmap

- DAG optimization passes (gate cancellation, commutation, fusion) and a
  scheduler that no longer relies on insertion order.
- A state panel in the GUI (amplitudes / Bloch-style plots via ImPlot3D).
- Alternative backends (e.g. stabilizer or tensor-network simulators).

## License

MIT © 2026 Breno Cunha Queiroz
