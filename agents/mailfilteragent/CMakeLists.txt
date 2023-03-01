# SPDX-FileCopyrightText: none
# SPDX-License-Identifier: BSD-3-Clause

add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_mailfilter_agent\")
add_executable(akonadi_mailfilter_agent)

qt_add_dbus_adaptor(akonadi_mailfilter_agent_SRCS org.freedesktop.Akonadi.MailFilterAgent.xml mailfilteragent.h MailFilterAgent)
target_sources(akonadi_mailfilter_agent PRIVATE
    dummykernel.cpp
    filterlogdialog.cpp
    filtermanager.cpp
    mailfilteragent.cpp
    mailfilterpurposemenuwidget.cpp
    dummykernel.h
    filterlogdialog.h
    filtermanager.h
    mailfilteragent.h
    mailfilterpurposemenuwidget.h
    ${akonadi_mailfilter_agent_SRCS}
    )



ecm_qt_declare_logging_category(akonadi_mailfilter_agent HEADER mailfilteragent_debug.h IDENTIFIER MAILFILTERAGENT_LOG CATEGORY_NAME org.kde.pim.mailfilteragent
        DESCRIPTION "kmail (mailfilter agent)"
        OLD_CATEGORY_NAMES log_mailfilteragent
        EXPORT KMAIL
    )


if (COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(akonadi_mailfilter_agent PROPERTIES UNITY_BUILD ON)
endif()

if( APPLE )
    set_target_properties(akonadi_mailfilter_agent PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${kmail_SOURCE_DIR}/agents/Info.plist.template)
    set_target_properties(akonadi_mailfilter_agent PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.mailfilter")
    set_target_properties(akonadi_mailfilter_agent PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi Email Filter")
endif ()


target_link_libraries(akonadi_mailfilter_agent
    KPim${KF_MAJOR_VERSION}::MailCommon
    KPim${KF_MAJOR_VERSION}::MessageComposer
    KF5::PimCommon
    KF5::AkonadiCore
    KPim${KF_MAJOR_VERSION}::AkonadiMime
    KF5::AkonadiAgentBase
    KPim${KF_MAJOR_VERSION}::Mime
    KPim${KF_MAJOR_VERSION}::IdentityManagement
    KF${KF_MAJOR_VERSION}::DBusAddons
    KF${KF_MAJOR_VERSION}::Notifications
    KF${KF_MAJOR_VERSION}::WindowSystem
    KF${KF_MAJOR_VERSION}::I18n
    )

install(TARGETS akonadi_mailfilter_agent ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES mailfilteragent.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents")
install(FILES akonadi_mailfilter_agent.notifyrc DESTINATION ${KDE_INSTALL_KNOTIFYRCDIR} )

