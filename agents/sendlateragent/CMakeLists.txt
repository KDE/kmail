# SPDX-FileCopyrightText: 2013-2025 Laurent Montel <montel@kde.org>
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


set(libsendlateragent_SRCS
    sendlaterconfigurewidget.cpp
    sendlatermanager.cpp
    sendlaterjob.cpp
    sendlaterutil.cpp
    sendlaterconfigurewidget.h
    sendlatermanager.h
    sendlaterjob.h
    sendlaterutil.h
    ${sendlater_common_SRCS}
)

set(sendlateragent_SRCS
    sendlateragent.cpp
    sendlateragent.h
)

qt_add_dbus_adaptor(sendlateragent_SRCS org.freedesktop.Akonadi.SendLaterAgent.xml sendlateragent.h SendLaterAgent)


ki18n_wrap_ui(libsendlateragent_SRCS ui/sendlaterconfigurewidget.ui)
kconfig_add_kcfg_files(libsendlateragent_SRCS sendlateragentsettings.kcfgc)
add_library(sendlateragent STATIC ${libsendlateragent_SRCS})
set_property(TARGET sendlateragent PROPERTY POSITION_INDEPENDENT_CODE ON)

#if(COMPILE_WITH_UNITY_CMAKE_SUPPORT)
#    set_target_properties(sendlateragent PROPERTIES UNITY_BUILD ON)
#endif()

target_link_libraries(sendlateragent
    KPim6::AkonadiCore
    KPim6::AkonadiMime
    KPim6::MailTransport
    KPim6::Mime
    KPim6::MessageComposer
    KPim6::MessageCore
    KPim6::PimCommon
    KF6::DBusAddons
    KF6::WindowSystem
    KF6::XmlGui
    KF6::Notifications
    KF6::I18n
)


add_executable(akonadi_sendlater_agent ${sendlateragent_SRCS})
target_link_libraries(akonadi_sendlater_agent
    sendlateragent
    KPim6::AkonadiAgentWidgetBase
)

if(APPLE)
    set_target_properties(akonadi_sendlater_agent PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${kmail_SOURCE_DIR}/agents/Info.plist.template)
    set_target_properties(akonadi_sendlater_agent PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi Send Later")
endif()

install(TARGETS akonadi_sendlater_agent ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES sendlateragent.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents")
install(FILES akonadi_sendlater_agent.notifyrc DESTINATION ${KDE_INSTALL_KNOTIFYRCDIR})


########## Configuration ##############

set(sendlateragentconfig_SRCS
    sendlaterinfoconfigwidget.cpp
    sendlaterinfoconfigwidget.h
    sendlaterremovemessagejob.cpp
    sendlaterremovemessagejob.h
)

kcoreaddons_add_plugin(sendlateragentconfig
    SOURCES ${sendlateragentconfig_SRCS}
    INSTALL_NAMESPACE pim6/akonadi/config
)
target_link_libraries(sendlateragentconfig
    sendlateragent
    KPim6::AkonadiCore
    KF6::XmlGui
    KF6::I18n
)

