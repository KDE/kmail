# SPDX-FileCopyrightText: none
# SPDX-License-Identifier: BSD-3-Clause
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
    unifiedmailbox.h
    unifiedmailboxagent.h
    unifiedmailboxmanager.h
    unifiedmailboxeditor.h
    settingsdialog.h
    mailkernel.h
    ${unifiedmailbox_agent_SRCS}
)

#if (COMPILE_WITH_UNITY_CMAKE_SUPPORT)
#    set_target_properties(akonadi_unifiedmailbox_agent PROPERTIES UNITY_BUILD ON)
#endif()
target_link_libraries(akonadi_unifiedmailbox_agent
    KPim${KF_MAJOR_VERSION}::AkonadiAgentBase
    KPim${KF_MAJOR_VERSION}::AkonadiMime
    KPim${KF_MAJOR_VERSION}::AkonadiWidgets
    KPim${KF_MAJOR_VERSION}::Mime
    KF${KF_MAJOR_VERSION}::I18n
    KPim${KF_MAJOR_VERSION}::IdentityManagement
    KF${KF_MAJOR_VERSION}::WidgetsAddons
    KF${KF_MAJOR_VERSION}::IconThemes
    KF${KF_MAJOR_VERSION}::ItemModels
    KPim${KF_MAJOR_VERSION}::MailCommon
    KF${KF_MAJOR_VERSION}::DBusAddons
)

if( APPLE )
    set_target_properties(akonadi_unifiedmailbox_agent PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${kmail_SOURCE_DIR}/agents/Info.plist.template)
    set_target_properties(akonadi_unifiedmailbox_agent PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.KF5::UnifiedMailbox")
    set_target_properties(akonadi_unifiedmailbox_agent PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE PIM Unified Mailbox")
endif ()

install(TARGETS akonadi_unifiedmailbox_agent ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )

install(FILES unifiedmailboxagent.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents")


