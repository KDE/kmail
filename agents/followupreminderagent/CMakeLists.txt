add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_followupreminder_agent\")

set(followupreminderagent_job_SRCS
    jobs/followupreminderjob.cpp
    jobs/followupreminderfinishtaskjob.cpp
    jobs/followupremindershowmessagejob.cpp
    )

set(followupreminderagent_SRCS
    followupreminderagent.cpp
    followupremindermanager.cpp
    followupreminderinfodialog.cpp
    followupremindernoanswerdialog.cpp
    followupreminderinfowidget.cpp
    ${followupreminderagent_job_SRCS}
    )

ecm_qt_declare_logging_category(followupreminderagent_SRCS HEADER followupreminderagent_debug.h IDENTIFIER FOLLOWUPREMINDERAGENT_LOG CATEGORY_NAME org.kde.pim.followupreminderagent)
qt5_add_dbus_adaptor(followupreminderagent_SRCS org.freedesktop.Akonadi.FollowUpReminder.xml followupreminderagent.h FollowUpReminderAgent)

add_executable(akonadi_followupreminder_agent ${followupreminderagent_SRCS})

target_link_libraries(akonadi_followupreminder_agent
    KF5::AkonadiCore
    KF5::AkonadiMime
    KF5::AkonadiAgentBase
    KF5::Mime
    KF5::CalendarCore
    KF5::DBusAddons
    KF5::FollowupReminder
    Qt5::Widgets
    )

if( APPLE )
    set_target_properties(akonadi_followupreminder_agent PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${kmail_SOURCE_DIR}/agents/Info.plist.template)
    set_target_properties(akonadi_followupreminder_agent PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.akonadi_followupreminder_agent")
    set_target_properties(akonadi_followupreminder_agent PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi Followup Reminder Agent")
endif ()

install(TARGETS akonadi_followupreminder_agent ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )

install(FILES followupreminder.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents")

if (BUILD_TESTING)
    add_subdirectory(autotests)
endif()

install(FILES akonadi_followupreminder_agent.notifyrc DESTINATION ${KDE_INSTALL_KNOTIFY5RCDIR} )
