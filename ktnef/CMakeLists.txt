
include(TestBigEndian)
test_big_endian(CMAKE_WORDS_BIGENDIAN)
configure_file(src/config-ktnef.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/src/config-ktnef.h)

add_subdirectory(src)

add_subdirectory(doc)
