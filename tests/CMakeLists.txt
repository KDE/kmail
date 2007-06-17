# This can be used for finding data files in the source dir, without installing them
add_definitions( -DKDESRCDIR=\\"${CMAKE_CURRENT_SOURCE_DIR}\\" )

include_directories( ${CMAKE_SOURCE_DIR}/mimelib ${CMAKE_SOURCE_DIR}/kmail  )

########### utiltests ###############

set(utiltests_SRCS utiltests.cpp ../util.cpp )
kde4_automoc(${utiltests_SRCS})
kde4_add_unit_test(utiltests TESTNAME kmail-utiltests ${utiltests_SRCS})
target_link_libraries(utiltests mimelib ${QT_QTTEST_LIBRARY} ${QT_QTCORE_LIBRARY})

########### mimelibtests ###############

set(mimelibtests_SRCS mimelibtests.cpp ../util.cpp )
kde4_automoc(${mimelibtests_SRCS})
kde4_add_unit_test(mimelibtests TESTNAME kmail-mimelibtests ${mimelibtests_SRCS})
target_link_libraries(mimelibtests mimelib ${QT_QTTEST_LIBRARY} ${QT_QTCORE_LIBRARY})

###### TODO port storagelayer tests to QTestLib

#kunittest_storagelayermodule_la_SOURCES = storagelayermodule.cpp messagedicttests.cpp ../kmdict.cpp

