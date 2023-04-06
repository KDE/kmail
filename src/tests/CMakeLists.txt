# SPDX-FileCopyrightText: none
# SPDX-License-Identifier: BSD-3-Clause
set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})

########### dbus test ###############
set(dbustest_SRCS dbustest.cpp)
qt_add_dbus_interfaces(dbustest_SRCS ${kmail_BINARY_DIR}/src/org.kde.kmail.kmail.xml)
add_executable(dbustest ${dbustest_SRCS})
add_dependencies(dbustest kmail_xml)
target_link_libraries(dbustest KF6::KIOCore KF6::I18n Qt::DBus)

#####
add_executable(searchdbustest  searchdbustest.cpp searchdbustest.h)
target_link_libraries(searchdbustest Qt::DBus Qt::Widgets KPim6::PimCommonAkonadi)

#####
add_executable(searchmailertest searchmailertest.cpp)
target_link_libraries(searchmailertest KPim6::MailCommon KPim6::LdapWidgets)

