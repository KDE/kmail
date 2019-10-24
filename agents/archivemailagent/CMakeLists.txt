add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_archivemail_agent\")

set(libarchivemailagent_SRCS
    archivemailkernel.cpp
    archivemailmanager.cpp
    archivemailinfo.cpp
    job/archivejob.cpp
    archivemailagentutil.cpp
    )

kconfig_add_kcfg_files(libarchivemailagent_SRCS
    settings/archivemailagentsettings.kcfgc
    )


qt5_add_dbus_adaptor(libarchivemailagent_SRCS org.freedesktop.Akonadi.ArchiveMailAgent.xml archivemailagent.h ArchiveMailAgent)

ecm_qt_declare_logging_category(libarchivemailagent_SRCS HEADER archivemailagent_debug.h IDENTIFIER ARCHIVEMAILAGENT_LOG CATEGORY_NAME org.kde.pim.archivemailagent)

ki18n_wrap_ui(libarchivemailagent_SRCS ui/archivemailwidget.ui )

add_library(archivemailagent STATIC ${libarchivemailagent_SRCS})
target_link_libraries(archivemailagent
    KF5::MailCommon
    KF5::I18n
    KF5::Notifications
    KF5::KIOWidgets
)

########################### Agent executable ################################

add_executable(akonadi_archivemail_agent archivemailagent.cpp)

target_link_libraries(akonadi_archivemail_agent
    archivemailagent
    KF5::AkonadiCore
    KF5::IdentityManagement
    KF5::AkonadiMime
    KF5::AkonadiAgentBase
    KF5::DBusAddons
    KF5::XmlGui
    KF5::KIOWidgets
    KF5::Notifications
    KF5::MailCommon
    KF5::Libkdepim
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


if (BUILD_TESTING)
    add_subdirectory(autotests)
endif()

########## Configuration ##############

set(archivemailagentconfig_SRCS
    addarchivemaildialog.cpp
    archivemailwidget.cpp
    widgets/formatcombobox.cpp
    widgets/unitcombobox.cpp
)

kcoreaddons_add_plugin(archivemailagentconfig
    JSON archivemailagentconfig.json
    SOURCES ${archivemailagentconfig_SRCS}
    INSTALL_NAMESPACE akonadi/config
)
target_link_libraries(archivemailagentconfig
    archivemailagent
    KF5::AkonadiCore
    KF5::XmlGui
    KF5::I18n
)

install(FILES settings/archivemailagentsettings.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})
