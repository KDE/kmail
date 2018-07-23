add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_unifiedmailbox_agent\")

if(BUILD_TESTING)
    #add_subdirectory(tests)
    #add_subdirectory(autotests)
endif()


set(unifiedmailbox_agent_SRCS
    unifiedmailboxagent.cpp
)
ecm_qt_declare_logging_category(unifiedmailbox_agent_SRCS HEADER unifiedmailboxagent_debug.h IDENTIFIER agent_log CATEGORY_NAME org.kde.pim.unifiedmailboxagent)
kconfig_add_kcfg_files(unifiedmailbox_agent_SRCS
    settings.kcfgc
)


add_executable(akonadi_unifiedmailbox_agent ${unifiedmailbox_agent_SRCS})

target_link_libraries(akonadi_unifiedmailbox_agent
    KF5::AkonadiAgentBase
    KF5::AkonadiMime
    KF5::Mime
    KF5::I18n
    KF5::IdentityManagement
)

if( APPLE )
    set_target_properties(akonadi_unifiedmailbox_agent PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${kmail_SOURCE_DIR}/agents/Info.plist.template)
    set_target_properties(akonadi_unifiedmailbox_agent PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.KF5::UnifiedMailbox")
    set_target_properties(akonadi_unifiedmailbox_agent PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE PIM Unified Mailbox")
endif ()

install(TARGETS akonadi_unifiedmailbox_agent ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )

install(FILES unifiedmailboxagent.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents")


