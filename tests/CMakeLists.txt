if(KDE4_BUILD_TESTS)

# Tests don't need to go into toplevel/bin, they are fine in the current dir.
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

# This can be used for finding data files in the source dir, without installing them
add_definitions( -DKDESRCDIR=\\"${CMAKE_CURRENT_SOURCE_DIR}\\" )

include_directories( ${CMAKE_SOURCE_DIR}/mimelib ${CMAKE_SOURCE_DIR}/kmail  )

########### utiltests ###############

set(utiltests_SRCS utiltests.cpp ../util.cpp )
kde4_automoc(${utiltests_SRCS})
kde4_add_test(utiltests ${utiltests_SRCS})
target_link_libraries(utiltests mimelib ${QT_QTTEST_LIBRARY} ${QT_QTCORE_LIBRARY})
add_test(kmail-utiltests ${EXECUTABLE_OUTPUT_PATH}/utiltests)

########### mimelibtests ###############

set(mimelibtests_SRCS mimelibtests.cpp ../util.cpp )
kde4_automoc(${mimelibtests_SRCS})
kde4_add_test(mimelibtests ${mimelibtests_SRCS})
target_link_libraries(mimelibtests mimelib ${QT_QTTEST_LIBRARY} ${QT_QTCORE_LIBRARY})
add_test(kmail-mimelibtests ${EXECUTABLE_OUTPUT_PATH}/mimelibtests)


###### TODO port storagelayer tests to QTestLib

#kunittest_storagelayermodule_la_SOURCES = storagelayermodule.cpp messagedicttests.cpp ../kmdict.cpp

endif(KDE4_BUILD_TESTS)
