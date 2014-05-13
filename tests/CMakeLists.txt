set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
include_directories(
  BEFORE
  ${CMAKE_SOURCE_DIR}/kmail
  ${CMAKE_BINARY_DIR}/kmail
)

########### dbus test ###############
set(dbustest_SRCS dbustest.cpp)
qt4_add_dbus_interfaces(dbustest_SRCS ${CMAKE_BINARY_DIR}/kmail/org.kde.kmail.kmail.xml)
kde4_add_executable(dbustest TEST ${dbustest_SRCS})
add_dependencies(dbustest kmail_xml)
target_link_libraries(dbustest ${KDE4_KIO_LIBS})


set( kmail_displaymenu_source displaymessageformatactionmenutest.cpp  )
kde4_add_unit_test( displaymessageformatactionmenutest ${kmail_displaymenu_source})
target_link_libraries( displaymessageformatactionmenutest ${QT_QTTEST_LIBRARY} ${KDE4_KDEUI_LIBS})


