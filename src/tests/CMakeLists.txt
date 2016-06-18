set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
include_directories(
  BEFORE
  ${CMAKE_SOURCE_DIR}/kmail
  ${CMAKE_BINARY_DIR}/kmail
)

########### dbus test ###############
set(dbustest_SRCS dbustest.cpp)
qt5_add_dbus_interfaces(dbustest_SRCS ${kmail_BINARY_DIR}/src/org.kde.kmail.kmail.xml)
add_executable(dbustest ${dbustest_SRCS})
add_dependencies(dbustest kmail_xml)
target_link_libraries(dbustest KF5::KIOCore KF5::I18n Qt5::DBus)

#####
set(searchdbustest_SRCS searchdbustest.cpp)
add_executable(searchdbustest ${searchdbustest_SRCS})
target_link_libraries(searchdbustest Qt5::DBus Qt5::Widgets KF5::PimCommon)
