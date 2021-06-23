add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_unifiedmailbox_agent\")

if(BUILD_TESTING)
    add_subdirectory(autotests)
endif()


add_executable(akonadi_unifiedmailbox_agent)
ecm_qt_declare_logging_category(akonadi_unifiedmailbox_agent HEADER unifiedmailboxagent_debug.h IDENTIFIER UNIFIEDMAILBOXAGENT_LOG CATEGORY_NAME org.kde.pim.unifiedmailboxagent
        DESCRIPTION "kmail (unifiedmailboxagent)"
        EXPORT KMAIL
    )

kconfig_add_kcfg_files(akonadi_unifiedmailbox_agent
    settings.kcfgc
)

qt_add_dbus_adaptor(unifiedmailbox_agent_SRCS org.freedesktop.Akonadi.UnifiedMailboxAgent.xml unifiedmailboxagent.h UnifiedMailboxAgent)
target_sources(akonadi_unifiedmailbox_agent PRIVATE
    unifiedmailbox.cpp
    unifiedmailboxagent.cpp
    unifiedmailboxmanager.cpp
    unifiedmailboxeditor.cpp
    settingsdialog.cpp
    mailkernel.cpp
    ${unifiedmailbox_agent_SRCS}
)

#if (COMPILE_WITH_UNITY_CMAKE_SUPPORT)
#    set_target_properties(akonadi_unifiedmailbox_agent PROPERTIES UNITY_BUILD ON)
#endif()
target_link_libraries(akonadi_unifiedmailbox_agent
    KF5::AkonadiAgentBase
    KF5::AkonadiMime
    KF5::AkonadiWidgets
    KF5::Mime
    KF5::I18n
    KF5::IdentityManagement
    KF5::WidgetsAddons
    KF5::IconThemes
    KF5::ItemModels
    KF5::MailCommon
    KF5::DBusAddons
)

if( APPLE )
    set_target_properties(akonadi_unifiedmailbox_agent PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${kmail_SOURCE_DIR}/agents/Info.plist.template)
    set_target_properties(akonadi_unifiedmailbox_agent PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.KF5::UnifiedMailbox")
    set_target_properties(akonadi_unifiedmailbox_agent PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE PIM Unified Mailbox")
endif ()

install(TARGETS akonadi_unifiedmailbox_agent ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )

install(FILES unifiedmailboxagent.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents")


