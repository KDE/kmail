# SPDX-FileCopyrightText: none
# SPDX-License-Identifier: BSD-3-Clause
########### next target ###############

set(kontact_summaryplugin_PART_SRCS summaryview_plugin.cpp summaryview_part.cpp dropwidget.cpp summaryview.qrc
    summaryview_plugin.h summaryview_part.h dropwidget.h)

qt_add_dbus_interfaces(kontact_summaryplugin_PART_SRCS ${kmail_BINARY_DIR}/src/org.kde.kmail.kmail.xml)

add_library(kontact_summaryplugin MODULE ${kontact_summaryplugin_PART_SRCS})
pim_target_precompile_headers(kontact_summaryplugin PUBLIC ../../../kmail_pch.h)
add_dependencies(kontact_summaryplugin kmail_xml)

target_link_libraries(kontact_summaryplugin KF5::KCMUtils KF5::I18n KF5::IdentityManagement KF5::KontactInterface KF5::PimCommon KF5::ConfigCore)

########### next target ###############

add_library(kcmkontactsummary MODULE kcmkontactsummary.cpp kcmkontactsummary.h)
pim_target_precompile_headers(kcmkontactsummary PUBLIC ../../../kmail_pch.h)

target_link_libraries(kcmkontactsummary KF5::I18n Qt${QT_MAJOR_VERSION}::Widgets KF5::I18n KF5::KontactInterface)

########### install files ###############

install(TARGETS kontact_summaryplugin DESTINATION ${KDE_INSTALL_PLUGINDIR}/kontact5)
install(TARGETS kcmkontactsummary DESTINATION ${KDE_INSTALL_PLUGINDIR}/pim/kcms/summary)

install(FILES kontactsummary_part.rc DESTINATION ${KDE_INSTALL_KXMLGUI5DIR}/kontactsummary)
