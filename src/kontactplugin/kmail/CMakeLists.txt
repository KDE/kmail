########### next target ###############
set(interface_SRCS)

set(kontact_kmail_plugins_interface_common_SRCS)
ecm_qt_declare_logging_category(kontact_kmail_plugins_interface_common_SRCS HEADER kmailplugin_debug.h IDENTIFIER KMAILPLUGIN_LOG CATEGORY_NAME org.kde.pim.kmail_plugin)

set(kontact_kmail_plugins_interface_SRCS ${interface_SRCS})
qt5_add_dbus_interfaces(kontact_kmail_plugins_interface_SRCS ${kmail_BINARY_DIR}/src/org.kde.kmail.kmail.xml)

set(kontact_kmailplugin_PART_SRCS kmail_plugin.cpp summarywidget.cpp ${kontact_kmail_plugins_interface_SRCS} ${kontact_kmail_plugins_interface_common_SRCS})

add_library(kontact_kmailplugin MODULE ${kontact_kmailplugin_PART_SRCS})
add_dependencies(kontact_kmailplugin kmail_xml)

target_link_libraries(kontact_kmailplugin KF5::Mime  KF5::KontactInterface KF5::CalendarCore KF5::CalendarUtils KF5::AkonadiCore KF5::Contacts KF5::AkonadiWidgets)

########### next target ###############

set(kcm_kmailsummary_PART_SRCS kcmkmailsummary.cpp ${kontact_kmail_plugins_interface_SRCS} ${kontact_kmail_plugins_interface_common_SRCS})

add_library(kcm_kmailsummary MODULE ${kcm_kmailsummary_PART_SRCS})
add_dependencies(kcm_kmailsummary kmail_xml)

target_link_libraries(kcm_kmailsummary  KF5::Mime KF5::PimCommon Qt5::DBus KF5::ConfigWidgets KF5::I18n)

########### install files ###############

install(TARGETS kontact_kmailplugin DESTINATION ${KDE_INSTALL_PLUGINDIR})

install(TARGETS kcm_kmailsummary DESTINATION ${KDE_INSTALL_PLUGINDIR})

install(FILES kmailplugin.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR}/kontact)

install(FILES kcmkmailsummary.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

install(FILES kmail.setdlg DESTINATION ${KDE_INSTALL_DATADIR}/kontact/ksettingsdialog)
