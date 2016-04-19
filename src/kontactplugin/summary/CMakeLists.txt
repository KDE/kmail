
########### next target ###############

set(kontact_summaryplugin_PART_SRCS summaryview_plugin.cpp summaryview_part.cpp dropwidget.cpp)
qt5_add_resources(kontact_summaryplugin_PART_SRCS summaryview.qrc)

qt5_add_dbus_interfaces(kontact_summaryplugin_PART_SRCS ${kmail_BINARY_DIR}/src/org.kde.kmail.kmail.xml)

add_library(kontact_summaryplugin MODULE ${kontact_summaryplugin_PART_SRCS})
add_dependencies(kontact_summaryplugin kmail_xml)

target_link_libraries(kontact_summaryplugin KF5::KCMUtils  KF5::IdentityManagement KF5::KontactInterface KF5::Libkdepim KF5::ConfigCore)

########### next target ###############

set(kcm_kontactsummary_PART_SRCS kcmkontactsummary.cpp)

add_library(kcm_kontactsummary MODULE ${kcm_kontactsummary_PART_SRCS})

target_link_libraries(kcm_kontactsummary KF5::KCMUtils Qt5::Widgets KF5::KCMUtils KF5::I18n KF5::KontactInterface)

########### install files ###############

install(TARGETS kontact_summaryplugin DESTINATION ${KDE_INSTALL_PLUGINDIR})
install(TARGETS kcm_kontactsummary DESTINATION ${KDE_INSTALL_PLUGINDIR})

install(FILES summaryplugin.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR}/kontact)
install(FILES kcmkontactsummary.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
install(FILES kontactsummary_part.rc DESTINATION ${KDE_INSTALL_KXMLGUI5DIR}/kontactsummary)

install(FILES summary.setdlg DESTINATION ${KDE_INSTALL_DATADIR}/kontact/ksettingsdialog)
