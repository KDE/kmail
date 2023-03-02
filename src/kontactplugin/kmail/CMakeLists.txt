# SPDX-FileCopyrightText: none
# SPDX-License-Identifier: BSD-3-Clause
########### next target ###############

set(kontact_kmail_plugins_interface_common_SRCS)

ecm_qt_declare_logging_category(kontact_kmail_plugins_interface_common_SRCS HEADER kmailplugin_debug.h IDENTIFIER KMAILPLUGIN_LOG CATEGORY_NAME org.kde.pim.kmail_plugin
        DESCRIPTION "kmail (kmail kontact plugins)"
        OLD_CATEGORY_NAMES log_kmail_plugin
        EXPORT KMAIL
    )


set(kontact_kmail_plugins_interface_SRCS)
qt_add_dbus_interfaces(kontact_kmail_plugins_interface_SRCS ${kmail_BINARY_DIR}/src/org.kde.kmail.kmail.xml)
configure_file(kmailplugin.json.cmake ${CMAKE_CURRENT_BINARY_DIR}/kmailplugin.json )
add_library(kontact_kmailplugin MODULE)
target_sources(kontact_kmailplugin PRIVATE kmail_plugin.cpp summarywidget.cpp kmail_plugin.h summarywidget.h ${kontact_kmail_plugins_interface_SRCS} ${kontact_kmail_plugins_interface_common_SRCS})
pim_target_precompile_headers(kontact_kmailplugin PUBLIC ../../../kmail_pch.h)
add_dependencies(kontact_kmailplugin kmail_xml)

target_link_libraries(kontact_kmailplugin KPim${KF_MAJOR_VERSION}::Mime KF${KF_MAJOR_VERSION}::I18n KPim${KF_MAJOR_VERSION}::KontactInterface KF${KF_MAJOR_VERSION}::CalendarCore KPim${KF_MAJOR_VERSION}::CalendarUtils KPim${KF_MAJOR_VERSION}::AkonadiCore KF${KF_MAJOR_VERSION}::Contacts KPim${KF_MAJOR_VERSION}::AkonadiWidgets)

########### next target ###############

add_library(kcmkmailsummary MODULE)
target_sources(kcmkmailsummary PRIVATE kcmkmailsummary.cpp kcmkmailsummary.h ${kontact_kmail_plugins_interface_SRCS} ${kontact_kmail_plugins_interface_common_SRCS})
pim_target_precompile_headers(kcmkmailsummary PUBLIC ../../../kmail_pch.h)

add_dependencies(kcmkmailsummary kmail_xml)

target_link_libraries(kcmkmailsummary  KPim${KF_MAJOR_VERSION}::Mime KF5::PimCommonAkonadi Qt::DBus KF${KF_MAJOR_VERSION}::ConfigWidgets KF${KF_MAJOR_VERSION}::I18n)

########### install files ###############

install(TARGETS kontact_kmailplugin DESTINATION ${KDE_INSTALL_PLUGINDIR}/pim${QT_MAJOR_VERSION}/kontact)

install(TARGETS kcmkmailsummary DESTINATION ${KDE_INSTALL_PLUGINDIR}/pim${QT_MAJOR_VERSION}/kcms/summary/)
