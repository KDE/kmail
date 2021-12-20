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

add_library(kontact_kmailplugin MODULE)
target_sources(kontact_kmailplugin PRIVATE kmail_plugin.cpp summarywidget.cpp kmail_plugin.h summarywidget.h ${kontact_kmail_plugins_interface_SRCS} ${kontact_kmail_plugins_interface_common_SRCS})
pim_target_precompile_headers(kontact_kmailplugin PUBLIC ../../../kmail_pch.h)
add_dependencies(kontact_kmailplugin kmail_xml)

target_link_libraries(kontact_kmailplugin KF5::Mime KF5::I18n KF5::KontactInterface KF5::CalendarCore KF5::CalendarUtils KF5::AkonadiCore KF5::Contacts KF5::AkonadiWidgets)

########### next target ###############

add_library(kcmkmailsummary MODULE)
target_sources(kcmkmailsummary PRIVATE kcmkmailsummary.cpp kcmkmailsummary.h ${kontact_kmail_plugins_interface_SRCS} ${kontact_kmail_plugins_interface_common_SRCS})
pim_target_precompile_headers(kcmkmailsummary PUBLIC ../../../kmail_pch.h)

add_dependencies(kcmkmailsummary kmail_xml)

target_link_libraries(kcmkmailsummary  KF5::Mime KF5::PimCommonAkonadi Qt::DBus KF5::ConfigWidgets KF5::I18n)

########### install files ###############

install(TARGETS kontact_kmailplugin DESTINATION ${KDE_INSTALL_PLUGINDIR}/kontact5)

install(TARGETS kcmkmailsummary DESTINATION ${KDE_INSTALL_PLUGINDIR}/pim/kcms/summary/)
