include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.15.2)
set(gtest_force_shared_crt
    ON
    CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Treat GoogleTest headers as system includes so our strict -Werror build is
# not broken by warnings originating inside the framework's headers.
foreach(_gtest_target gtest gtest_main gmock gmock_main)
  if(TARGET ${_gtest_target})
    get_target_property(_dirs ${_gtest_target} INTERFACE_INCLUDE_DIRECTORIES)
    if(_dirs)
      set_target_properties(
        ${_gtest_target} PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES
                                    "${_dirs}")
    endif()
  endif()
endforeach()

include(GoogleTest)

add_executable(
  ket_tests
  test_sanity.cpp
  test_qubit.cpp
  test_gate.cpp
  test_circuit.cpp
  test_dag.cpp
  test_block.cpp
  test_simulator.cpp
  test_measurement.cpp
  test_observable.cpp
  test_stabilizer.cpp
  test_parallelism.cpp
  test_fusion.cpp
  test_print.cpp
  test_qasm.cpp)
target_link_libraries(ket_tests PRIVATE ket ket_warnings GTest::gtest_main)

# The CLI command tests are only buildable when the CLI core library is built.
if(KET_CLI)
  target_sources(ket_tests PRIVATE test_cli.cpp)
  target_link_libraries(ket_tests PRIVATE ket_cli_core)
endif()

gtest_discover_tests(ket_tests)
