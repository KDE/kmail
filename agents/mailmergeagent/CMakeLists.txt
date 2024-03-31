# SPDX-FileCopyrightText: 2021-2024 Laurent Montel <montel@kde.org>
# SPDX-License-Identifier: BSD-3-Clause

add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_mailmerge_agent\")
if(BUILD_TESTING)
    add_subdirectory(tests)
    add_subdirectory(autotests)
endif()
add_library(mailmergeagent STATIC)
set_property(TARGET mailmergeagent PROPERTY POSITION_INDEPENDENT_CODE ON)


ecm_qt_declare_logging_category(mailmergeagent HEADER mailmergeagent_debug.h IDENTIFIER MAILMERGEAGENT_LOG CATEGORY_NAME org.kde.pim.mailmergeagent
        DESCRIPTION "kmail (mailmergeagent)"
        OLD_CATEGORY_NAMES log_mailmergeagent
        EXPORT KMAIL
    )

qt_add_dbus_adaptor(mailmergeagent_SRCS org.freedesktop.Akonadi.MailMergeAgent.xml mailmergeagent.h MailMergeAgent)

ki18n_wrap_ui(mailmergeagent ui/mailmergeconfigurewidget.ui)

target_sources(mailmergeagent PRIVATE
    mailmergeagent.cpp
    mailmergemanager.cpp
    mailmergeconfiguredialog.cpp
    mailmergeconfigurewidget.cpp
    mailmergeutil.cpp
    mailmergeagent.h
    mailmergemanager.h
    mailmergeconfiguredialog.h
    mailmergeconfigurewidget.h
    mailmergeutil.h
    ${mailmergeagent_SRCS}
    )

kconfig_add_kcfg_files(mailmergeagent mailmergeagentsettings.kcfgc)
#if (COMPILE_WITH_UNITY_CMAKE_SUPPORT)
#    set_target_properties(sendlateragent PROPERTIES UNITY_BUILD ON)
#endif()

target_link_libraries(mailmergeagent
    KPim6::AkonadiCore
    KPim6::AkonadiMime
    #KPim6::Mime
    KPim6::MessageComposer
    KPim6::MessageCore
    KF6::DBusAddons
    KF6::WindowSystem
    KF6::XmlGui
    KF6::I18n
    KF6::Notifications
    KPim6::AkonadiAgentBase
    )

add_executable(akonadi_mailmerge_agent)
target_link_libraries(akonadi_mailmerge_agent
    mailmergeagent
    KPim6::AkonadiAgentBase
    )



if( APPLE )
    set_target_properties(akonadi_mailmerge_agent PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${kmail_SOURCE_DIR}/agents/Info.plist.template)
    set_target_properties(akonadi_mailmerge_agent PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi Mail Merge")
endif ()

install(TARGETS akonadi_mailmerge_agent ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )

install(FILES mailmergeagent.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents")
install(FILES akonadi_mailmerge_agent.notifyrc DESTINATION ${KDE_INSTALL_KNOTIFYRCDIR} )
akonadi_install_systemd_unit(AGENT TARGET akonadi_mailmerge_agent DESCRIPTION "Akonadi Mail Merge Agent")

#kcoreaddons_add_plugin(mailmergeagentconfig
#    SOURCES ${mailmergeagentconfig_SRCS}
#    INSTALL_NAMESPACE pim6/akonadi/config
#    )
