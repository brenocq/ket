// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <random>

#include <ket/ket.hpp>

namespace py = pybind11;

PYBIND11_MODULE(_ket, m) {
  m.doc() = "Ket: a quantum computing library";

  py::class_<ket::StateVector>(m, "StateVector",
                               "A quantum state vector of 2^n complex amplitudes.")
      .def("__len__", &ket::StateVector::size)
      .def(
          "__getitem__",
          [](const ket::StateVector& s, std::size_t i) {
            if (i >= s.size()) throw py::index_error();
            return s[i];
          },
          py::arg("index"))
      .def("print", &ket::StateVector::print,
           "Render the amplitudes in ket notation.")
      .def("__str__", &ket::StateVector::print)
      .def("__repr__", [](const ket::StateVector& s) {
        return "<ket.StateVector size=" + std::to_string(s.size()) + ">";
      });

  py::class_<ket::Circuit>(m, "Circuit", "A quantum circuit (a DAG of gates).")
      .def(py::init<std::size_t, std::string>(), py::arg("n_qubits"),
           py::arg("name") = "")
      .def_property_readonly("n_qubits", &ket::Circuit::n_qubits)
      .def_property_readonly("n_clbits", &ket::Circuit::n_clbits)
      .def_property_readonly("name", &ket::Circuit::name)
      .def("h", py::overload_cast<std::size_t>(&ket::Circuit::h),
           py::arg("qubit"), "Apply a Hadamard gate.")
      .def("x", py::overload_cast<std::size_t>(&ket::Circuit::x),
           py::arg("qubit"), "Apply a Pauli-X gate.")
      .def("z", py::overload_cast<std::size_t>(&ket::Circuit::z),
           py::arg("qubit"), "Apply a Pauli-Z gate.")
      .def("cnot", py::overload_cast<std::size_t, std::size_t>(&ket::Circuit::cnot),
           py::arg("control"), py::arg("target"), "Apply a controlled-NOT gate.")
      .def("barrier",
           py::overload_cast<const std::string&>(&ket::Circuit::barrier),
           py::arg("label") = "", "Add a barrier across all qubits.")
      .def("barrier",
           py::overload_cast<const std::vector<std::size_t>&, const std::string&>(
               &ket::Circuit::barrier),
           py::arg("qubits"), py::arg("label") = "",
           "Add a barrier across a subset of qubits.")
      .def("measure", &ket::Circuit::measure, py::arg("qubit"), py::arg("clbit"),
           "Measure a qubit into a classical bit.")
      .def("measure_all", &ket::Circuit::measure_all,
           "Measure every qubit i into classical bit i.")
      .def("append", &ket::Circuit::append, py::arg("sub"), py::arg("qubits"),
           py::arg("name") = "",
           "Append a sub-circuit as a single composite block.")
      .def("decompose", &ket::Circuit::decompose,
           "Return a copy with composite blocks expanded one level.")
      .def("print", &ket::Circuit::print, "Render the circuit as ASCII art.")
      .def("__str__", &ket::Circuit::print)
      .def("__repr__", [](const ket::Circuit& c) {
        return "<ket.Circuit n_qubits=" + std::to_string(c.n_qubits()) + ">";
      });

  m.def("run", &ket::run, py::arg("circuit"),
        "Simulate the circuit and return the resulting state vector.");

  m.def(
      "measure",
      [](const ket::StateVector& state, std::optional<std::uint32_t> seed) {
        if (seed.has_value()) {
          std::mt19937 rng{*seed};
          return ket::measure(state, rng);
        }
        return ket::measure(state);
      },
      py::arg("state"), py::arg("seed") = py::none(),
      "Sample a basis-state index from the state vector (Born rule). "
      "Pass `seed` for a reproducible result.");

  m.def(
      "sample",
      [](const ket::Circuit& circuit, std::optional<std::uint32_t> seed) {
        if (seed.has_value()) {
          std::mt19937 rng{*seed};
          return ket::sample(circuit, rng);
        }
        return ket::sample(circuit);
      },
      py::arg("circuit"), py::arg("seed") = py::none(),
      "Run the circuit and sample one shot of its measurements, returning the "
      "classical register. Pass `seed` for a reproducible result.");
}
