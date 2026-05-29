include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.15.2
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

add_executable(ket_tests
  test_sanity.cpp
  test_qubit.cpp
  test_gate.cpp
  test_circuit.cpp
  test_simulator.cpp
)
target_link_libraries(ket_tests PRIVATE ket GTest::gtest_main)
gtest_discover_tests(ket_tests)
