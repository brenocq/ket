cmake_minimum_required(VERSION 3.20)
project(ket CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Strict warnings, applied only to our own targets (not fetched dependencies).
add_library(ket_warnings INTERFACE)
if(MSVC)
  target_compile_options(ket_warnings INTERFACE /W4 /WX /permissive-)
else()
  target_compile_options(ket_warnings INTERFACE
    -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Werror)
endif()

add_library(ket
  src/ket.cpp
  src/circuit.cpp
  src/dag.cpp
  src/simulator.cpp
  src/measurement.cpp
)
target_include_directories(ket PUBLIC include)
target_link_libraries(ket PRIVATE ket_warnings)

enable_testing()
add_subdirectory(tests)
