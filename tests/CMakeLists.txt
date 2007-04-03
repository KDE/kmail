if(KDE4_BUILD_TESTS)

# Tests don't need to go into toplevel/bin, they are fine in the current dir.
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

include_directories( ${CMAKE_SOURCE_DIR}/mimelib ${CMAKE_SOURCE_DIR}/kmail  )


########### utiltests ###############

set(utiltests_SRCS utiltests.cpp ../util.cpp )

kde4_automoc(${utiltests_SRCS})

kde4_add_executable(utiltests NOGUI RUN_UNINSTALLED ${utiltests_SRCS})

target_link_libraries(utiltests mimelib ${QT_QTTEST_LIBRARY} ${QT_QTCORE_LIBRARY})

add_test(kmail-utiltests ${EXECUTABLE_OUTPUT_PATH}/utiltests)



###### TODO port storagelayer tests to QTestLib

#kunittest_storagelayermodule_la_SOURCES = storagelayermodule.cpp messagedicttests.cpp ../kmdict.cpp

endif(KDE4_BUILD_TESTS)
