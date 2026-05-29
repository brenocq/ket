add_executable(bell bell.cpp)
target_link_libraries(bell PRIVATE ket ket_warnings)

add_executable(ghz ghz.cpp)
target_link_libraries(ghz PRIVATE ket ket_warnings)
