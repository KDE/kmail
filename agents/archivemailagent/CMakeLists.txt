project(archivemailagent)

add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_archivemail_agent\")

set(archivemailagent_SRCS
    archivemailkernel.cpp
    archivemailagent.cpp
    archivemailmanager.cpp
    archivemaildialog.cpp
    archivemailinfo.cpp
    addarchivemaildialog.cpp
    archivemailwidget.cpp
    job/archivejob.cpp
    archivemailagentutil.cpp
    widgets/formatcombobox.cpp
    widgets/unitcombobox.cpp
    )

kconfig_add_kcfg_files(archivemailagent_SRCS
    settings/archivemailagentsettings.kcfgc
    )


qt5_add_dbus_adaptor(archivemailagent_SRCS org.freedesktop.Akonadi.ArchiveMailAgent.xml archivemailagent.h ArchiveMailAgent)

ecm_qt_declare_logging_category(archivemailagent_SRCS HEADER archivemailagent_debug.h IDENTIFIER ARCHIVEMAILAGENT_LOG CATEGORY_NAME org.kde.pim.archivemailagent)


ki18n_wrap_ui(archivemailagent_SRCS ui/archivemailwidget.ui )

add_executable(akonadi_archivemail_agent ${archivemailagent_SRCS})

target_link_libraries(akonadi_archivemail_agent
    KF5::AkonadiCore
    KF5::IdentityManagement
    KF5::AkonadiMime
    KF5::AkonadiAgentBase
    KF5::DBusAddons
    KF5::XmlGui
    KF5::WindowSystem
    KF5::KIOWidgets
    KF5::Notifications
    KF5::MailCommon
    KF5::Libkdepim
    KF5::IconThemes
    KF5::I18n
    )

if( APPLE )
    set_target_properties(akonadi_archivemail_agent PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${kmail_SOURCE_DIR}/agents/Info.plist.template)
    set_target_properties(akonadi_archivemail_agent PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.archivemail")
    set_target_properties(akonadi_archivemail_agent PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi Email Archiver")
endif ()

install(TARGETS akonadi_archivemail_agent ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )

install(FILES archivemailagent.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents")
install(FILES akonadi_archivemail_agent.notifyrc DESTINATION ${KDE_INSTALL_KNOTIFY5RCDIR} )

install(FILES settings/archivemailagentsettings.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})

if (BUILD_TESTING)
    add_subdirectory(autotests)
endif()
