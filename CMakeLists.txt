cmake_minimum_required(VERSION 3.20)
project(ket CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(KET_TESTS "Build the C++ test suite" OFF)
option(KET_PYTHON "Build the Python bindings" OFF)
option(KET_EXAMPLES "Build the example programs" OFF)

# Strict warnings, applied only to our own targets (not fetched dependencies).
add_library(ket_warnings INTERFACE)
if(MSVC)
  target_compile_options(ket_warnings INTERFACE /W4 /WX /permissive-)
else()
  target_compile_options(
    ket_warnings
    INTERFACE -Wall
              -Wextra
              -Wpedantic
              -Wshadow
              -Wconversion
              -Wsign-conversion
              -Werror)
endif()

add_library(ket src/ket.cpp src/circuit.cpp src/dag.cpp src/simulator.cpp
                src/measurement.cpp)
target_include_directories(ket PUBLIC include)
target_link_libraries(ket PRIVATE ket_warnings)

if(KET_PYTHON)
  # The static core is linked into a shared extension module, so it must be
  # position-independent.
  set_target_properties(ket PROPERTIES POSITION_INDEPENDENT_CODE ON)

  # In an isolated pip/scikit-build-core build pybind11 is provided as a build
  # dependency and found via its CMake config; for raw CMake dev builds we fall
  # back to fetching it.
  find_package(pybind11 CONFIG QUIET)
  if(NOT pybind11_FOUND)
    include(FetchContent)
    FetchContent_Declare(
      pybind11
      GIT_REPOSITORY https://github.com/pybind/pybind11.git
      GIT_TAG v2.13.6)
    FetchContent_MakeAvailable(pybind11)
  endif()

  pybind11_add_module(_ket bindings/module.cpp)
  target_link_libraries(_ket PRIVATE ket)
  install(TARGETS _ket LIBRARY DESTINATION ket)
endif()

if(KET_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

if(KET_EXAMPLES)
  add_subdirectory(examples)
endif()
