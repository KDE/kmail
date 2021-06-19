########### next target ###############

set(kontact_kmail_plugins_interface_common_SRCS)

ecm_qt_declare_logging_category(kontact_kmail_plugins_interface_common_SRCS HEADER kmailplugin_debug.h IDENTIFIER KMAILPLUGIN_LOG CATEGORY_NAME org.kde.pim.kmail_plugin
        DESCRIPTION "kmail (kmail kontact plugins)"
        OLD_CATEGORY_NAMES log_kmail_plugin
        EXPORT KMAIL
    )


set(kontact_kmail_plugins_interface_SRCS)
qt_add_dbus_interfaces(kontact_kmail_plugins_interface_SRCS ${kmail_BINARY_DIR}/src/org.kde.kmail.kmail.xml)

set(kontact_kmailplugin_PART_SRCS kmail_plugin.cpp summarywidget.cpp ${kontact_kmail_plugins_interface_SRCS} ${kontact_kmail_plugins_interface_common_SRCS})

add_library(kontact_kmailplugin MODULE ${kontact_kmailplugin_PART_SRCS})
kcoreaddons_desktop_to_json(kontact_kmailplugin "kmailplugin.desktop" SERVICE_TYPES kcmodule.desktop)
pim_target_precompile_headers(kontact_kmailplugin PUBLIC ../../../kmail_pch.h)
add_dependencies(kontact_kmailplugin kmail_xml)

target_link_libraries(kontact_kmailplugin KF5::Mime KF5::I18n KF5::KontactInterface KF5::CalendarCore KF5::CalendarUtils KF5::AkonadiCore KF5::Contacts KF5::AkonadiWidgets)

########### next target ###############

set(kcmkmailsummary_PART_SRCS kcmkmailsummary.cpp ${kontact_kmail_plugins_interface_SRCS} ${kontact_kmail_plugins_interface_common_SRCS})

add_library(kcmkmailsummary MODULE ${kcmkmailsummary_PART_SRCS})
pim_target_precompile_headers(kcmkmailsummary PUBLIC ../../../kmail_pch.h)
kcoreaddons_desktop_to_json(kcmkmailsummary "kcmkmailsummary.desktop" SERVICE_TYPES kcmodule.desktop)

add_dependencies(kcmkmailsummary kmail_xml)

target_link_libraries(kcmkmailsummary  KF5::Mime KF5::PimCommonAkonadi Qt::DBus KF5::ConfigWidgets KF5::I18n)

########### install files ###############

install(TARGETS kontact_kmailplugin DESTINATION ${KDE_INSTALL_PLUGINDIR}/kontact5)

install(TARGETS kcmkmailsummary DESTINATION ${KDE_INSTALL_PLUGINDIR}/pim/kcms/summary/)

install(FILES kmailplugin.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR}/kontact)

install(FILES kcmkmailsummary.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

install(FILES kmail.setdlg DESTINATION ${KDE_INSTALL_DATADIR}/kontact/ksettingsdialog)
