set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
include_directories(
  BEFORE
  ${CMAKE_SOURCE_DIR}/kmail
  ${CMAKE_BINARY_DIR}/kmail
)

########### dbus test ###############
set(dbustest_SRCS dbustest.cpp)
qt5_add_dbus_interfaces(dbustest_SRCS ${CMAKE_BINARY_DIR}/kmail/org.kde.kmail.kmail.xml)
add_executable(dbustest ${dbustest_SRCS})
add_dependencies(dbustest kmail_xml)
target_link_libraries(dbustest KF5::KIOCore KF5::I18n Qt5::DBus)


set( kmail_displaymenu_source displaymessageformatactionmenutest.cpp ../widgets/displaymessageformatactionmenu.cpp )
add_executable( displaymessageformatactionmenutest ${kmail_displaymenu_source} )
add_test( displaymessageformatactionmenutest displaymessageformatactionmenutest )
ecm_mark_as_test(kmail-displaymessageformatactionmenutest)
target_link_libraries( displaymessageformatactionmenutest Qt5::Test  messageviewer)

set( kmail_identityaddvcarddialog_source identityaddvcarddialogtest.cpp ../identity/identityaddvcarddialog.cpp )
add_executable( identityaddvcarddialogtest  ${kmail_identityaddvcarddialog_source})
ecm_mark_as_test(kmail-identityaddvcarddialogtest)
target_link_libraries( identityaddvcarddialogtest Qt5::Test Qt5::Widgets KF5::Completion KF5::KIOWidgets KF5::I18n)

set( kmail_followupreminderselectdatedialog_source followupreminderselectdatedialogtest.cpp ../followupreminder/followupreminderselectdatedialog.cpp )
add_executable( followupreminderselectdatedialogtest ${kmail_followupreminderselectdatedialog_source})
ecm_mark_as_test(kmail-followupreminderselectdatedialogtest)
target_link_libraries( followupreminderselectdatedialogtest Qt5::Test KF5::KIOWidgets KF5::I18n KF5::CalendarCore KF5::AkonadiCore KF5::AkonadiWidgets )



set( kmail_statusbarlabeltoggledstate_source statusbarlabeltoggledstatetest.cpp ../widgets/statusbarlabeltoggledstate.cpp )
add_executable(statusbarlabeltoggledstatetest ${kmail_statusbarlabeltoggledstate_source})
ecm_mark_as_test(kmail-statusbarlabeltoggledstatetest)
target_link_libraries( statusbarlabeltoggledstatetest Qt5::Test KF5::KIOWidgets KF5::I18n)


set( kmail_createfollowupreminderonexistingmessagejobtest_source createfollowupreminderonexistingmessagejobtest.cpp ../job/createfollowupreminderonexistingmessagejob.cpp ../followupreminder/followupremindercreatejob.cpp)
add_executable( createfollowupreminderonexistingmessagejobtest ${kmail_createfollowupreminderonexistingmessagejobtest_source})
ecm_mark_as_test(kmail_createfollowupreminderonexistingmessagejobtest)
target_link_libraries( createfollowupreminderonexistingmessagejobtest Qt5::Test Qt5::Widgets KF5::AkonadiCore KF5::CalendarCore KF5::Mime followupreminder)

