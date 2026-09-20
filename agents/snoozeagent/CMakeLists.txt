# SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>
# SPDX-License-Identifier: BSD-3-Clause
add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_snooze_agent\")

if(BUILD_TESTING)
    add_subdirectory(autotests)
endif()

set(snooze_common_SRCS)
ecm_qt_declare_logging_category(snooze_common_SRCS HEADER snoozeagent_debug.h IDENTIFIER SNOOZEAGENT_LOG CATEGORY_NAME org.kde.pim.snoozeagent
        DESCRIPTION "kmail (snoozeagent)"
        EXPORT KMAIL
)

set(libsnoozeagent_SRCS
    job/snoozewakeupjob.cpp
    job/snoozewakeupjob.h
    snoozeattribute.cpp
    snoozeattribute.h
    snoozeconfigurewidget.cpp
    snoozeconfigurewidget.h
    snoozeinfo.cpp
    snoozeinfo.h
    snoozemanager.cpp
    snoozemanager.h
    snoozeutil.cpp
    snoozeutil.h
    ${snooze_common_SRCS}
)

set(snoozeagent_SRCS
    snoozeagent.cpp
    snoozeagent.h
)

qt_add_dbus_adaptor(snoozeagent_SRCS org.freedesktop.Akonadi.SnoozeAgent.xml snoozeagent.h SnoozeAgent)

kconfig_add_kcfg_files(libsnoozeagent_SRCS snoozeagentsettings.kcfgc)
add_library(snoozeagent STATIC ${libsnoozeagent_SRCS})
target_include_directories(
    snoozeagent
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
)

# precompiled headers support
if(ENABLE_PCH)
    target_precompile_headers(
        snoozeagent
        PRIVATE
            <QObject>
            <QString>
            <QStringList>
            <QList>
            <QTimer>
            <QDateTime>
            <QWidget>
            <QVBoxLayout>
            <KLocalizedString>
            <KConfigGroup>
            <KSharedConfig>
            <memory>
    )
endif()
set_property(
    TARGET
        snoozeagent
    PROPERTY
        POSITION_INDEPENDENT_CODE
            ON
)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/snoozeagentsettings.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})

target_link_libraries(
    snoozeagent
    KPim6::AkonadiCore
    KPim6::AkonadiMime
    KF6::Mime
    KF6::ConfigCore
    KF6::ConfigGui
    KF6::DBusAddons
    KF6::Notifications
    KF6::I18n
    KF6::WidgetsAddons
    Qt::Widgets
    Qt::DBus
)

add_executable(akonadi_snooze_agent ${snoozeagent_SRCS})
target_link_libraries(
    akonadi_snooze_agent
    snoozeagent
    KPim6::AkonadiAgentWidgetBase
)

if(APPLE)
    set_target_properties(
        akonadi_snooze_agent
        PROPERTIES
            MACOSX_BUNDLE_INFO_PLIST
                ${kmail_SOURCE_DIR}/agents/Info.plist.template
    )
    set_target_properties(
        akonadi_snooze_agent
        PROPERTIES
            MACOSX_BUNDLE_BUNDLE_NAME
                "KDE Akonadi Snooze"
    )
endif()

install(
    TARGETS
        akonadi_snooze_agent
        ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}
)

install(FILES snoozeagent.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents")
install(FILES akonadi_snooze_agent.notifyrc DESTINATION ${KDE_INSTALL_KNOTIFYRCDIR})

########## Configuration ##############

set(snoozeagentconfig_SRCS
    snoozeinfoconfigwidget.cpp
    snoozeinfoconfigwidget.h
)

kcoreaddons_add_plugin(snoozeagentconfig
    SOURCES ${snoozeagentconfig_SRCS}
    INSTALL_NAMESPACE pim6/akonadi/config
)
target_link_libraries(
    snoozeagentconfig
    snoozeagent
    KPim6::AkonadiCore
    KF6::I18n
)
