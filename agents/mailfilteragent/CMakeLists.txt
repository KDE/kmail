project(akonadi_mailfilter_agent)

add_subdirectory( kconf_update )

add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_mailfilter_agent\")

set(akonadi_mailfilter_agent_SRCS
    dummykernel.cpp
    filterlogdialog.cpp
    filtermanager.cpp
    mailfilteragent.cpp
    configuredialog.cpp
    configurewidget.cpp
    )

qt5_add_dbus_adaptor(akonadi_mailfilter_agent_SRCS org.freedesktop.Akonadi.MailFilterAgent.xml mailfilteragent.h MailFilterAgent)

ecm_qt_declare_logging_category(akonadi_mailfilter_agent_SRCS HEADER mailfilteragent_debug.h IDENTIFIER MAILFILTERAGENT_LOG CATEGORY_NAME org.kde.pim.mailfilteragent)

add_executable(akonadi_mailfilter_agent ${akonadi_mailfilter_agent_SRCS})

if( APPLE )
    set_target_properties(akonadi_mailfilter_agent PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${kmail_SOURCE_DIR}/agents/Info.plist.template)
    set_target_properties(akonadi_mailfilter_agent PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.mailfilter")
    set_target_properties(akonadi_mailfilter_agent PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi Email Filter")
endif ()


target_link_libraries(akonadi_mailfilter_agent
    KF5::MailCommon
    KF5::MessageComposer
    KF5::PimCommon
    KF5::AkonadiCore
    KF5::AkonadiMime
    KF5::AkonadiAgentBase
    KF5::Mime
    KF5::IdentityManagement
    KF5::DBusAddons
    KF5::Notifications
    KF5::WindowSystem
    KF5::IconThemes
    KF5::Libkdepim
    KF5::I18n
    )

install(TARGETS akonadi_mailfilter_agent ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES mailfilteragent.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents")
install(FILES akonadi_mailfilter_agent.notifyrc DESTINATION ${KDE_INSTALL_KNOTIFY5RCDIR} )

if(BUILD_TESTING)
    add_subdirectory(autotests)
endif()

