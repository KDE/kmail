# SPDX-FileCopyrightText: none
# SPDX-License-Identifier: BSD-3-Clause
########### next target ###############

set(kontact_summaryplugin_PART_SRCS summaryview_plugin.cpp summaryview_part.cpp dropwidget.cpp summaryview.qrc
    summaryview_plugin.h summaryview_part.h dropwidget.h)

qt_add_dbus_interfaces(kontact_summaryplugin_PART_SRCS ${kmail_BINARY_DIR}/src/org.kde.kmail.kmail.xml)
configure_file(summaryplugin.json.cmake ${CMAKE_CURRENT_BINARY_DIR}/summaryplugin.json)

add_library(kontact_summaryplugin MODULE ${kontact_summaryplugin_PART_SRCS})
pim_target_precompile_headers(kontact_summaryplugin PUBLIC ../../../kmail_pch.h)
add_dependencies(kontact_summaryplugin kmail_xml)

target_link_libraries(kontact_summaryplugin KF${KF_MAJOR_VERSION}::KCMUtils KF${KF_MAJOR_VERSION}::I18n KF5::IdentityManagement KF5::KontactInterface KF5::PimCommon KF${KF_MAJOR_VERSION}::ConfigCore)

########### next target ###############

add_library(kcmkontactsummary MODULE kcmkontactsummary.cpp kcmkontactsummary.h)
pim_target_precompile_headers(kcmkontactsummary PUBLIC ../../../kmail_pch.h)

target_link_libraries(kcmkontactsummary KF${KF_MAJOR_VERSION}::I18n Qt::Widgets KF${KF_MAJOR_VERSION}::I18n KF5::KontactInterface)

########### install files ###############

install(TARGETS kontact_summaryplugin DESTINATION ${KDE_INSTALL_PLUGINDIR}/pim${QT_MAJOR_VERSION}/kontact)
install(TARGETS kcmkontactsummary DESTINATION ${KDE_INSTALL_PLUGINDIR}/pim${QT_MAJOR_VERSION}/kcms/summary)

install(FILES kontactsummary_part.rc DESTINATION ${KDE_INSTALL_KXMLGUIDIR}/kontactsummary)
