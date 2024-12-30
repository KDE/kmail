# SPDX-FileCopyrightText: 2014-2024 Laurent Montel <montel@kde.org>
# SPDX-License-Identifier: BSD-3-Clause
add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_followupreminder_agent\")

add_library(followupreminderagent STATIC)
set_property(TARGET followupreminderagent PROPERTY POSITION_INDEPENDENT_CODE ON)



kconfig_add_kcfg_files(followupreminderagent
        followupreminderagentsettings.kcfgc
)

ecm_qt_declare_logging_category(followupreminderagent HEADER followupreminderagent_debug.h IDENTIFIER FOLLOWUPREMINDERAGENT_LOG CATEGORY_NAME org.kde.pim.followupreminderagent
        DESCRIPTION "kmail (followupreminderagent)"
        OLD_CATEGORY_NAMES log_followupreminderagent
        EXPORT KMAIL
)

qt_add_dbus_adaptor(followupreminderagent_SRCS org.freedesktop.Akonadi.FollowUpReminder.xml followupreminderagent.h FollowUpReminderAgent)

qt_add_dbus_interface(followupreminderagent_SRCS org.freedesktop.Notifications.xml notifications_interface)
qt_add_dbus_interface(followupreminderagent_SRCS org.freedesktop.DBus.Properties.xml dbusproperties)

target_sources(followupreminderagent PRIVATE
    followupremindermanager.cpp
    followupremindernoanswerdialog.cpp
    followupreminderinfowidget.cpp
    followupreminderinfo.cpp
    followupreminderutil.cpp
    jobs/followupreminderjob.cpp
    jobs/followupreminderfinishtaskjob.cpp
    jobs/followupremindershowmessagejob.cpp

    followupremindermanager.h
    followupremindernoanswerdialog.h
    followupreminderinfowidget.h
    followupreminderinfo.h
    followupreminderutil.h
    jobs/followupreminderjob.h
    jobs/followupreminderfinishtaskjob.h
    jobs/followupremindershowmessagejob.h

    ${followupreminderagent_SRCS}
)

#if(COMPILE_WITH_UNITY_CMAKE_SUPPORT)
#    set_target_properties(followupreminderagent PROPERTIES UNITY_BUILD ON)
#endif()

target_link_libraries(followupreminderagent
    KPim6::AkonadiCore
    KPim6::IdentityManagementCore
    KPim6::AkonadiMime
    KPim6::AkonadiAgentBase
    KF6::DBusAddons
    KF6::XmlGui
    KF6::KIOWidgets
    KF6::Notifications
    KPim6::MailCommon
    KF6::I18n
    KF6::CalendarCore
)


########################### Agent executable ################################
add_executable(akonadi_followupreminder_agent followupreminderagent.cpp followupreminderagent.h)

target_link_libraries(akonadi_followupreminder_agent
    followupreminderagent
    KPim6::AkonadiCore
    KPim6::AkonadiMime
    KPim6::AkonadiAgentBase
    KPim6::Mime
    KF6::CalendarCore
    KF6::DBusAddons
    Qt::Widgets
    KF6::Notifications
    KF6::I18n
    KF6::XmlGui
    KF6::Service
)

if(APPLE)
    set_target_properties(akonadi_followupreminder_agent PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${kmail_SOURCE_DIR}/agents/Info.plist.template)
    set_target_properties(akonadi_followupreminder_agent PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.akonadi_followupreminder_agent")
    set_target_properties(akonadi_followupreminder_agent PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi Followup Reminder Agent")
endif()

install(TARGETS akonadi_followupreminder_agent ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES followupreminder.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents")

if(BUILD_TESTING)
    add_subdirectory(autotests)
endif()

install(FILES akonadi_followupreminder_agent.notifyrc DESTINATION ${KDE_INSTALL_KNOTIFYRCDIR})

########## Configuration ##############

set(followupreminderagentconfig_SRCS
    followupreminderinfoconfigwidget.cpp
    followupreminderinfoconfigwidget.h
)

kcoreaddons_add_plugin(followupreminderagentconfig
    SOURCES ${followupreminderagentconfig_SRCS}
    INSTALL_NAMESPACE pim6/akonadi/config
)
target_link_libraries(followupreminderagentconfig
    followupreminderagent
    KPim6::AkonadiCore
    KF6::XmlGui
    KF6::I18n
)
