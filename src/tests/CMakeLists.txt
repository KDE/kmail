set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})

########### dbus test ###############
set(dbustest_SRCS dbustest.cpp)
qt_add_dbus_interfaces(dbustest_SRCS ${kmail_BINARY_DIR}/src/org.kde.kmail.kmail.xml)
add_executable(dbustest ${dbustest_SRCS})
add_dependencies(dbustest kmail_xml)
target_link_libraries(dbustest KF5::KIOCore KF5::I18n Qt::DBus)

#####
add_executable(searchdbustest  searchdbustest.cpp)
target_link_libraries(searchdbustest Qt::DBus Qt::Widgets KF5::PimCommonAkonadi)

#####
add_executable(searchmailertest searchmailertest.cpp)
target_link_libraries(searchmailertest KF5::MailCommon KF5::Ldap)

