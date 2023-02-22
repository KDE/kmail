# SPDX-FileCopyrightText: 2021-2023 Laurent Montel <montel@kde.org>
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
    KF5::AkonadiCore
    KF5::AkonadiMime
    KF5::MailTransportAkonadi
    #KPim${KF_MAJOR_VERSION}::Mime
    KF5::MessageComposer
    KF5::MessageCore
    KF${KF_MAJOR_VERSION}::DBusAddons
    KF${KF_MAJOR_VERSION}::WindowSystem
    KF${KF_MAJOR_VERSION}::XmlGui
    KF${KF_MAJOR_VERSION}::I18n
    KF${KF_MAJOR_VERSION}::Notifications
    KF5::AkonadiAgentBase
    )

add_executable(akonadi_mailmerge_agent)
target_link_libraries(akonadi_mailmerge_agent
    mailmergeagent
    KF5::AkonadiAgentBase
    )



if( APPLE )
    set_target_properties(akonadi_mailmerge_agent PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${kmail_SOURCE_DIR}/agents/Info.plist.template)
    set_target_properties(akonadi_mailmerge_agent PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi Mail Merge")
endif ()

install(TARGETS akonadi_mailmerge_agent ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )

install(FILES mailmergeagent.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents")
install(FILES akonadi_mailmerge_agent.notifyrc DESTINATION ${KDE_INSTALL_KNOTIFYRCDIR} )

#kcoreaddons_add_plugin(mailmergeagentconfig
#    SOURCES ${mailmergeagentconfig_SRCS}
#    INSTALL_NAMESPACE pim${QT_MAJOR_VERSION}/akonadi/config
#    )
