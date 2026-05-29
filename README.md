# Ket

**Ket** (|⟩) is a quantum computing library for C++. You describe a quantum
circuit, add gates to it, simulate it on a built-in state-vector simulator, and
sample measurement outcomes.

Ket represents a circuit as a **directed acyclic graph (DAG)** of gates: each
node is a gate, and edges connect gates that share a qubit, capturing the
order in which they must execute. This makes gate dependencies explicit and
leaves room for future analysis and optimization passes.

> Python bindings are planned but not yet available.

## Status

This is an early, intentionally small implementation. It can build circuits
out of a handful of basic gates and simulate them exactly.

### Available gates

| Method                      | Gate                | Qubits |
| --------------------------- | ------------------- | ------ |
| `c.h(q)`                    | Hadamard            | 1      |
| `c.x(q)`                    | Pauli-X (NOT)       | 1      |
| `c.z(q)`                    | Pauli-Z             | 1      |
| `c.cnot(control, target)`   | Controlled-NOT      | 2      |


## How the simulation works

Ket uses a **full state-vector simulator**. A register of *n* qubits is
represented by a vector of `2ⁿ` complex amplitudes, initialized to the
ground state |0…0⟩.

Each gate is applied as an in-place linear transformation over the state
vector:

- **Single-qubit gates** (H, X, Z) apply a 2×2 unitary to every pair of
  amplitudes that differ only in the target qubit's bit.
- **CNOT** swaps the amplitudes of the basis states whose control bit is set,
  toggling the target bit.

Qubit `i` corresponds to bit `i` of the basis-state index (little-endian).
The simulator walks the circuit's DAG in topological order — which, by
construction, matches insertion order — and applies each gate in turn.

**Measurement** samples a basis state according to the Born rule: outcome `i`
occurs with probability `|amplitudeᵢ|²`. Sampling uses the inverse-CDF method
with a uniform random draw. You can pass your own `std::mt19937` for
reproducible results, or omit it to use a per-thread random seed.

## Example

```cpp
#include <ket/ket.hpp>
#include <iostream>

int main() {
  // Build a Bell state: H on q0, then CNOT(q0 -> q1).
  ket::Circuit c{2};
  c.h(0);
  c.cnot(0, 1);

  std::cout << c.print();

  // Simulate and inspect the amplitudes.
  ket::StateVector state = ket::run(c);
  for (std::size_t i = 0; i < state.size(); ++i) {
    std::cout << i << ": " << state[i] << '\n';
  }

  // Sample 1000 measurement outcomes.
  std::mt19937 rng{42};
  int counts[4] = {0, 0, 0, 0};
  for (int shot = 0; shot < 1000; ++shot) {
    ++counts[ket::measure(state, rng)];
  }
  for (int i = 0; i < 4; ++i) {
    std::cout << "|" << i << ">: " << counts[i] << '\n';
  }
}
```

### Circuit print output

`c.print()` renders the circuit as a Qiskit-style ASCII diagram:

```
     ┌───┐
q_0: ┤ H ├──■──
     └───┘┌─┴─┐
q_1: ─────┤ X ├
          └───┘
```

### Simulation output

The Bell circuit produces the entangled state (|00⟩ + |11⟩)/√2:

```
0: (0.707107,0)
1: (0,0)
2: (0,0)
3: (0.707107,0)
```

Sampling collapses onto the two correlated outcomes in roughly equal
proportions, and never onto |01⟩ or |10⟩:

```
|0>: 493
|1>: 0
|2>: 0
|3>: 507
```

## Building

Ket uses CMake and requires a C++20 compiler. Tests are built with GoogleTest,
which is fetched automatically.

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

To use Ket in your own project, link against the `ket` library target and add
`include/` to your include path. The umbrella header `<ket/ket.hpp>` pulls in
the whole public API.

## Limitations

- **State-vector only.** Memory and time grow as `2ⁿ`, so simulation is
  practical only for a small number of qubits (~20–25).
- **Small gate set.** Only H, X, Z, and CNOT are implemented.
- **No parameterized or multi-controlled gates** (e.g. rotations, Toffoli).
- **Measurement does not collapse** the state — it samples from a fixed state
  vector, which is ideal for shot-style experiments but not for mid-circuit
  measurement with feed-forward.
- **No circuit-level measurement instruction yet** — measurement operates on a
  simulated state vector rather than being recorded in the circuit/DAG.
- **No noise modeling.**
- **No layer packing in the diagram** — each gate occupies its own column.

## Future work

- More gates: Y, S, T, controlled-Z, and parameterized rotations (Rx, Ry, Rz).
- Multi-controlled gates (Toffoli, etc.).
- A circuit-level measurement instruction, with a classical register rendered
  in the diagram.
- DAG optimization passes (gate cancellation, commutation, fusion) — and a
  scheduler that no longer relies on insertion order.
- Layer packing in the ASCII renderer.
- Python bindings.
- Alternative backends (e.g. stabilizer or tensor-network simulators).

## License

MIT © 2026 Breno Cunha Queiroz
