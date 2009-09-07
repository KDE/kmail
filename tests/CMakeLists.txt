set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

include_directories(
  ${CMAKE_SOURCE_DIR}/mimelib
  ${CMAKE_SOURCE_DIR}/kmail
  ${Boost_INCLUDE_DIRS}
)

########### htmlquotecolorertest ###############
set(htmlquotecolorertest_SRCS htmlquotecolorertest.cpp ../htmlquotecolorer.cpp)
kde4_add_unit_test(htmlquotecolorertest TESTNAME kmail-quotecolorertest ${htmlquotecolorertest_SRCS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}")
target_link_libraries(htmlquotecolorertest
  ${QT_QTTEST_LIBRARY}
  ${QT_QTCORE_LIBRARY}
  ${KDE4_KHTML_LIBRARY}
)

########### utiltests ###############

set(utiltests_SRCS utiltests.cpp ../util.cpp)
kde4_add_unit_test(utiltests TESTNAME kmail-utiltests ${utiltests_SRCS})
target_link_libraries(utiltests mimelib
  ${QT_QTTEST_LIBRARY}
  ${QT_QTCORE_LIBRARY}
  ${KDE4_KIO_LIBS}
  ${KDEPIMLIBS_MAILTRANSPORT_LIBS}
)

########### messagedicttests ###############
set(messagedicttests_SRCS messagedicttests.cpp ../kmdict.cpp)
kde4_add_unit_test(messagedicttests TESTNAME kmail-messagedicttests ${messagedicttests_SRCS})
target_link_libraries(messagedicttests ${QT_QTTEST_LIBRARY} ${QT_QTCORE_LIBRARY}
                      ${KDE4_KIO_LIBS})

########### mimelibtests ###############

set(mimelibtests_SRCS mimelibtests.cpp ../util.cpp)
kde4_add_unit_test(mimelibtests TESTNAME kmail-mimelibtests ${mimelibtests_SRCS})
target_link_libraries(mimelibtests mimelib
  ${QT_QTTEST_LIBRARY}
  ${QT_QTCORE_LIBRARY}
  ${KDE4_KIO_LIBS}
  ${KDEPIMLIBS_MAILTRANSPORT_LIBS}
)

########### stringutiltest ###############

set(stringutiltest_SRCS stringutiltest.cpp ../stringutil.cpp)
add_definitions(-DKMAIL_UNITTESTS=YES)
kde4_add_unit_test(stringutiltest TESTNAME kmail-stringutiltest ${stringutiltest_SRCS})
target_link_libraries(stringutiltest
  ${QT_QTTEST_LIBRARY}
  ${QT_QTCORE_LIBRARY}
  ${KDE4_KIO_LIBS}
)

########### dbus test ###############
set(dbustest_SRCS dbustest.cpp)
qt4_add_dbus_interfaces( dbustest_SRCS ${CMAKE_BINARY_DIR}/kmail/org.kde.kmail.kmail.xml)
kde4_add_executable(dbustest TEST ${dbustest_SRCS})
add_dependencies(dbustest kmail_xml)
target_link_libraries(dbustest  ${KDE4_KIO_LIBS} )

########### recipientseditor test ###############

set(recipienteditortest_SRCS recipientseditortest.cpp )


kde4_add_executable(recipienteditortest TEST ${recipienteditortest_SRCS})

target_link_libraries(recipienteditortest  ${KDE4_KIO_LIBS} kmailprivate kdepim )


###### TODO port storagelayer tests to QTestLib

#kunittest_storagelayermodule_la_SOURCES = storagelayermodule.cpp messagedicttests.cpp ../kmdict.cpp

