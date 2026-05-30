cmake_minimum_required(VERSION 3.20)

# Single source of truth for the version; also read by pyproject.toml.
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" KET_VERSION LIMIT_COUNT 1)
project(
  ket
  VERSION ${KET_VERSION}
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)

option(KET_TESTS "Build the C++ test suite" OFF)
option(KET_PYTHON "Build the Python bindings" OFF)
option(KET_EXAMPLES "Build the example programs" OFF)
option(KET_CLI "Build the ket-cli command-line executable" OFF)
option(KET_GUI "Build the ket-gui graphical executable" OFF)

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

# Generate the version header so the C++ constant always matches VERSION.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/ket/version.hpp.in
               ${PROJECT_BINARY_DIR}/include/ket/version.hpp @ONLY)

add_library(
  ket
  src/ket.cpp
  src/circuit.cpp
  src/dag.cpp
  src/state.cpp
  src/simulator.cpp
  src/measurement.cpp
  src/observable.cpp
  bindings/qasm/qasm.cpp)
# Include dirs differ between the build tree and an install: consumers in this
# build see the source + generated headers; installed consumers see the
# installed include directory.
target_include_directories(
  ket
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
         $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
# Build-only: the warning flags must not leak into the exported target.
target_link_libraries(ket PRIVATE $<BUILD_INTERFACE:ket_warnings>)
# Propagate the C++20 requirement to consumers of the installed target.
target_compile_features(ket PUBLIC cxx_std_20)

# Install + CMake package config, so consumers can `find_package(ket)` and link
# `ket::ket`. Skipped for the Python wheel build, which ships only the module.
if(NOT KET_PYTHON)
  include(CMakePackageConfigHelpers)

  install(
    TARGETS ket
    EXPORT ketTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

  install(
    DIRECTORY include/ket
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING
    PATTERN "*.hpp")
  install(FILES ${PROJECT_BINARY_DIR}/include/ket/version.hpp
          DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ket)

  install(
    EXPORT ketTargets
    FILE ketTargets.cmake
    NAMESPACE ket::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ket)

  configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ketConfig.cmake.in
    ${PROJECT_BINARY_DIR}/ketConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ket)
  write_basic_package_version_file(
    ${PROJECT_BINARY_DIR}/ketConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)
  install(FILES ${PROJECT_BINARY_DIR}/ketConfig.cmake
                ${PROJECT_BINARY_DIR}/ketConfigVersion.cmake
          DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ket)
endif()

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

  pybind11_add_module(_ket bindings/python/module.cpp)
  target_link_libraries(_ket PRIVATE ket)
  install(TARGETS _ket LIBRARY DESTINATION ket)
endif()

if(KET_CLI)
  add_subdirectory(cli)
endif()

if(KET_GUI)
  add_subdirectory(gui)
endif()

if(KET_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

if(KET_EXAMPLES)
  add_subdirectory(examples)
endif()
