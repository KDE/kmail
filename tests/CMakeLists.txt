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


set( kmail_statusbarlabeltoggledstate_source statusbarlabeltoggledstatetest.cpp ../widgets/statusbarlabeltoggledstate.cpp )
kde4_add_unit_test( statusbarlabeltoggledstatetest ${kmail_statusbarlabeltoggledstate_source})
target_link_libraries( statusbarlabeltoggledstatetest ${QT_QTTEST_LIBRARY} ${KDE4_KDEUI_LIBS})


set( kmail_createfollowupreminderonexistingmessagejobtest_source createfollowupreminderonexistingmessagejobtest.cpp ../job/createfollowupreminderonexistingmessagejob.cpp ../followupreminder/followupremindercreatejob.cpp)
kde4_add_unit_test( createfollowupreminderonexistingmessagejobtest ${kmail_createfollowupreminderonexistingmessagejobtest_source})
target_link_libraries( createfollowupreminderonexistingmessagejobtest ${QT_QTTEST_LIBRARY} ${KDE4_KDEUI_LIBS} ${KDEPIMLIBS_AKONADI_LIBS} ${KDEPIMLIBS_KCALCORE_LIBS} followupreminder)


set( kmail_cryptostateindicatorwidgettest_source cryptostateindicatorwidgettest.cpp ../editor/widgets/cryptostateindicatorwidget.cpp)
kde4_add_unit_test( cryptostateindicatorwidgettest ${kmail_cryptostateindicatorwidgettest_source})
target_link_libraries( cryptostateindicatorwidgettest ${QT_QTTEST_LIBRARY} ${KDE4_KDEUI_LIBS} messagecore)

set( kmail_kactionmenutransporttest_source kactionmenutransporttest.cpp ../widgets/kactionmenutransport.cpp)
kde4_add_unit_test( kactionmenutransporttest ${kmail_kactionmenutransporttest_source})
target_link_libraries( kactionmenutransporttest ${QT_QTTEST_LIBRARY} ${KDE4_KDEUI_LIBS} ${KDEPIMLIBS_MAILTRANSPORT_LIBS})

set( kmail_kactionmenuchangecasetest_source kactionmenuchangecasetest.cpp ../widgets/kactionmenuchangecase.cpp)
kde4_add_unit_test( kactionmenuchangecasetest ${kmail_kactionmenuchangecasetest_source})
target_link_libraries( kactionmenuchangecasetest ${QT_QTTEST_LIBRARY} ${KDE4_KDEUI_LIBS})


include(AkonadiMacros)
set(KDEPIMLIBS_RUN_ISOLATED_TESTS TRUE)
set(KDEPIMLIBS_RUN_SQLITE_ISOLATED_TESTS TRUE)

add_akonadi_isolated_test_advanced( followupreminderselectdatedialogtest.cpp  "../followupreminder/followupreminderselectdatedialog.cpp" "${KDEPIMLIBS_AKONADI_CALENDAR_LIBS};${KDEPIMLIBS_KCALCORE_LIBS}")


