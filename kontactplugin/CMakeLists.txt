include_directories(
  ${CMAKE_SOURCE_DIR}/kmail
  ${CMAKE_BINARY_DIR}/kmail
  ${Boost_INCLUDE_DIRS}
)

########### next target ###############
set(interface_SRCS)

set(kontact_kmail_plugins_interface_SRCS ${interface_SRCS})
qt5_add_dbus_interfaces(kontact_kmail_plugins_interface_SRCS ${CMAKE_BINARY_DIR}/kmail/org.kde.kmail.kmail.xml)

set(kontact_kmailplugin_PART_SRCS kmailplugin_debug.cpp kmail_plugin.cpp summarywidget.cpp ${kontact_kmail_plugins_interface_SRCS})

add_library(kontact_kmailplugin MODULE ${kontact_kmailplugin_PART_SRCS})
add_dependencies(kontact_kmailplugin kmail_xml)

target_link_libraries(kontact_kmailplugin KF5::Mime  KF5::KontactInterface KF5::CalendarCore KF5::CalendarUtils kdepim)

########### next target ###############

set(kcm_kmailsummary_PART_SRCS kmailplugin_debug.cpp kcmkmailsummary.cpp ${kontact_kmail_plugins_interface_SRCS})

add_library(kcm_kmailsummary MODULE ${kcm_kmailsummary_PART_SRCS})
add_dependencies(kcm_kmailsummary kmail_xml)

target_link_libraries(kcm_kmailsummary  KF5::Mime KF5::AkonadiCore  pimcommon)

########### install files ###############

install(TARGETS kontact_kmailplugin DESTINATION ${KDE_INSTALL_PLUGINDIR})

install(TARGETS kcm_kmailsummary DESTINATION ${KDE_INSTALL_PLUGINDIR})

install(FILES kmailplugin.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR}/kontact)

install(FILES kcmkmailsummary.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

install(FILES kmail.setdlg DESTINATION ${KDE_INSTALL_DATADIR}/kontact/ksettingsdialog)
