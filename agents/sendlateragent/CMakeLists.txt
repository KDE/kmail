# SPDX-FileCopyrightText: 2013-2021 Laurent Montel <montel@kde.org>
# SPDX-License-Identifier: BSD-3-Clause
add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_sendlater_agent\")

if(BUILD_TESTING)
    add_subdirectory(tests)
    add_subdirectory(autotests)
endif()

set(sendlater_common_SRCS)
ecm_qt_declare_logging_category(sendlater_common_SRCS HEADER sendlateragent_debug.h IDENTIFIER SENDLATERAGENT_LOG CATEGORY_NAME org.kde.pim.sendlateragent
        DESCRIPTION "kmail (sendlateragent)"
        OLD_CATEGORY_NAMES log_sendlateragent
        EXPORT KMAIL
    )


set(sendlateragentconfig_SRCS
    sendlaterconfiguredialog.cpp
    sendlaterconfigurewidget.cpp
    )

set(libsendlateragent_SRCS
    ${sendlateragentconfig_SRCS}
    ${sendlater_common_SRCS}
    sendlatermanager.cpp
    sendlaterjob.cpp
    sendlaterremovemessagejob.cpp
    sendlaterutil.cpp
    )

set(sendlateragent_SRCS
    sendlateragent.cpp
    )

qt_add_dbus_adaptor(sendlateragent_SRCS org.freedesktop.Akonadi.SendLaterAgent.xml sendlateragent.h SendLaterAgent)


ki18n_wrap_ui(libsendlateragent_SRCS ui/sendlaterconfigurewidget.ui)
kconfig_add_kcfg_files(libsendlateragent_SRCS sendlateragentsettings.kcfgc)
add_library(sendlateragent STATIC ${libsendlateragent_SRCS})
pim_target_precompile_headers(sendlateragent PUBLIC ../../kmail_pch.h)
#if (COMPILE_WITH_UNITY_CMAKE_SUPPORT)
#    set_target_properties(sendlateragent PROPERTIES UNITY_BUILD ON)
#endif()

target_link_libraries(sendlateragent
    KF5::AkonadiCore
    KF5::AkonadiMime
    KF5::MailTransportAkonadi
    KF5::Mime
    KF5::MessageComposer
    KF5::MessageCore
    KF5::PimCommon
    KF5::DBusAddons
    KF5::WindowSystem
    KF5::XmlGui
    KF5::Notifications
    KF5::I18n
    )


add_executable(akonadi_sendlater_agent ${sendlateragent_SRCS})
target_link_libraries(akonadi_sendlater_agent
    sendlateragent
    KF5::AkonadiAgentBase
    )

if( APPLE )
    set_target_properties(akonadi_sendlater_agent PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${kmail_SOURCE_DIR}/agents/Info.plist.template)
    set_target_properties(akonadi_sendlater_agent PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi Send Later")
endif ()

install(TARGETS akonadi_sendlater_agent ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )

install(FILES sendlateragent.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents")
install(FILES akonadi_sendlater_agent.notifyrc DESTINATION ${KDE_INSTALL_KNOTIFY5RCDIR} )

#kcoreaddons_add_plugin(sendlateragentconfig
#    JSON sendlateragentconfig.json
#    SOURCES ${sendlateragentconfig_SRCS}
#    INSTALL_NAMESPACE akonadi/config
#)
