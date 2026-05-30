# The command implementations live in a small static library so they can be
# linked into both the executable and the test suite.
add_library(ket_cli_core STATIC commands.cpp)
target_link_libraries(ket_cli_core PUBLIC ket)
target_link_libraries(ket_cli_core PRIVATE ket_warnings)
target_include_directories(ket_cli_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# The library target is named `ket`, so the executable target uses a distinct
# name and only its output file is `ket-cli`.
add_executable(ket_cli main.cpp)
set_target_properties(ket_cli PROPERTIES OUTPUT_NAME ket-cli)
target_link_libraries(ket_cli PRIVATE ket_cli_core ket_warnings)
