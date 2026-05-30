add_executable(bell bell.cpp)
target_link_libraries(bell PRIVATE ket ket_warnings)

add_executable(ghz ghz.cpp)
target_link_libraries(ghz PRIVATE ket ket_warnings)

add_executable(deutsch_jozsa deutsch_jozsa.cpp)
target_link_libraries(deutsch_jozsa PRIVATE ket ket_warnings)

add_executable(bernstein_vazirani bernstein_vazirani.cpp)
target_link_libraries(bernstein_vazirani PRIVATE ket ket_warnings)

add_executable(bit_flip_code bit_flip_code.cpp)
target_link_libraries(bit_flip_code PRIVATE ket ket_warnings)

add_executable(grover grover.cpp)
target_link_libraries(grover PRIVATE ket ket_warnings)
