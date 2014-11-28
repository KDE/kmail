set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
include_directories(
  BEFORE
  ${CMAKE_SOURCE_DIR}/kmail
  ${CMAKE_BINARY_DIR}/kmail
)

########### dbus test ###############
set(dbustest_SRCS dbustest.cpp)
qt4_add_dbus_interfaces(dbustest_SRCS ${CMAKE_BINARY_DIR}/kmail/org.kde.kmail.kmail.xml)
kde4_add_executable(dbustest TEST ${dbustest_SRCS})
add_dependencies(dbustest kmail_xml)
target_link_libraries(dbustest ${KDE4_KIO_LIBS})


set( kmail_displaymenu_source displaymessageformatactionmenutest.cpp ../widgets/displaymessageformatactionmenu.cpp )
kde4_add_unit_test( displaymessageformatactionmenutest ${kmail_displaymenu_source})
target_link_libraries( displaymessageformatactionmenutest ${QT_QTTEST_LIBRARY} ${KDE4_KDEUI_LIBS} messageviewer)


set( kmail_identityaddvcarddialog_source identityaddvcarddialogtest.cpp ../identity/identityaddvcarddialog.cpp )
kde4_add_unit_test( identityaddvcarddialogtest ${kmail_identityaddvcarddialog_source})
target_link_libraries( identityaddvcarddialogtest ${QT_QTTEST_LIBRARY} ${KDE4_KDEUI_LIBS} ${KDE4_KFILE_LIBS})


set( kmail_followupreminderselectdatedialog_source followupreminderselectdatedialogtest.cpp ../followupreminder/followupreminderselectdatedialog.cpp )
kde4_add_unit_test( followupreminderselectdatedialogtest ${kmail_followupreminderselectdatedialog_source})
target_link_libraries( followupreminderselectdatedialogtest ${QT_QTTEST_LIBRARY} ${KDE4_KDEUI_LIBS} ${KDEPIMLIBS_AKONADI_LIBS} ${KDEPIMLIBS_AKONADI_CALENDAR_LIBS}  ${KDEPIMLIBS_KCALCORE_LIBS})


set( kmail_statusbarlabeltoggledstate_source statusbarlabeltoggledstatetest.cpp ../widgets/statusbarlabeltoggledstate.cpp )
kde4_add_unit_test( statusbarlabeltoggledstatetest ${kmail_statusbarlabeltoggledstate_source})
target_link_libraries( statusbarlabeltoggledstatetest ${QT_QTTEST_LIBRARY} ${KDE4_KDEUI_LIBS})


set( kmail_createfollowupreminderonexistingmessagejobtest_source createfollowupreminderonexistingmessagejobtest.cpp ../job/createfollowupreminderonexistingmessagejob.cpp ../followupreminder/followupremindercreatejob.cpp)
kde4_add_unit_test( createfollowupreminderonexistingmessagejobtest ${kmail_createfollowupreminderonexistingmessagejobtest_source})
target_link_libraries( createfollowupreminderonexistingmessagejobtest ${QT_QTTEST_LIBRARY} ${KDE4_KDEUI_LIBS} ${KDEPIMLIBS_AKONADI_LIBS} ${KDEPIMLIBS_KCALCORE_LIBS} followupreminder)

