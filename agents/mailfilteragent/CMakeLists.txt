# SPDX-FileCopyrightText: none
# SPDX-License-Identifier: BSD-3-Clause

add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_mailfilter_agent\")

add_library(mailfilteragentprivate)


target_sources(mailfilteragentprivate PRIVATE
    filterlogdialog.cpp
    filterlogdialog.h
    mailfilterpurposemenuwidget.cpp
    mailfilterpurposemenuwidget.h

    mailfilteragent_private_export.h
)

if (COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(mailfilteragentprivate PROPERTIES UNITY_BUILD ON)
endif()

generate_export_header(mailfilteragentprivate BASE_NAME mailfilteragentprivate)
target_link_libraries(mailfilteragentprivate
    KF6::I18n
    Qt::Widgets
    KF6::TextCustomEditor
    KPim6::PimCommon
    KPim6::MailCommon
)

set_target_properties(mailfilteragentprivate
    PROPERTIES OUTPUT_NAME mailfilteragentprivate VERSION ${KDEPIM_LIB_VERSION} SOVERSION ${KDEPIM_LIB_SOVERSION}
)
install(TARGETS mailfilteragentprivate ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)


add_executable(akonadi_mailfilter_agent)
set(dbusobjectmanager_xml org.freedesktop.Akonadi.MailFilterAgent.xml)
set_source_files_properties(${dbusobjectmanager_xml} PROPERTIES INCLUDE "dbusoperators.h")
qt_add_dbus_adaptor(akonadi_mailfilter_agent_SRCS ${dbusobjectmanager_xml} mailfilteragent.h MailFilterAgent)

target_sources(akonadi_mailfilter_agent PRIVATE
    dummykernel.cpp
    filtermanager.cpp
    mailfilteragent.cpp
    dummykernel.h
    filtermanager.h
    mailfilteragent.h
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
    KPim6::MailCommon
    KPim6::MessageComposer
    KPim6::PimCommon
    KPim6::AkonadiCore
    KPim6::AkonadiMime
    KPim6::AkonadiAgentBase
    KPim6::Mime
    KPim6::IdentityManagementCore
    KF6::DBusAddons
    KF6::Notifications
    KF6::WindowSystem
    KF6::I18n
    mailfilteragentprivate
    )

install(TARGETS akonadi_mailfilter_agent ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES mailfilteragent.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents")
install(FILES akonadi_mailfilter_agent.notifyrc DESTINATION ${KDE_INSTALL_KNOTIFYRCDIR} )
akonadi_install_systemd_unit(AGENT TARGET akonadi_mailfilter_agent DESCRIPTION "Akonadi Mail Filter Agent")

add_subdirectory(tests)
