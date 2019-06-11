
include(TestBigEndian)
test_big_endian(CMAKE_WORDS_BIGENDIAN)
configure_file(src/config-ktnef.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/src/config-ktnef.h)

add_subdirectory(src)

if (${ECM_VERSION} STRGREATER "5.58.0")
    install(FILES ktnefapps.categories ktnefapps.renamecategories  DESTINATION  ${KDE_INSTALL_LOGGINGCATEGORIESDIR})
else()
    install(FILES ktnefapps.categories ktnefapps.renamecategories  DESTINATION ${KDE_INSTALL_CONFDIR})
endif()

add_subdirectory(doc)
