set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
include_directories(
  BEFORE
  ${CMAKE_SOURCE_DIR}/kmail
  ${CMAKE_BINARY_DIR}/kmail
)

########### dbus test ###############
set( kmail_displaymenu_source displaymessageformatactionmenutest.cpp ../widgets/displaymessageformatactionmenu.cpp )
add_executable( displaymessageformatactionmenutest ${kmail_displaymenu_source} )
add_test( displaymessageformatactionmenutest displaymessageformatactionmenutest )
ecm_mark_as_test(kmail-displaymessageformatactionmenutest)
target_link_libraries( displaymessageformatactionmenutest Qt5::Test  messageviewer)

set( kmail_identityaddvcarddialog_source identityaddvcarddialogtest.cpp ../identity/identityaddvcarddialog.cpp )
add_executable( identityaddvcarddialogtest  ${kmail_identityaddvcarddialog_source})
ecm_mark_as_test(kmail-identityaddvcarddialogtest)
target_link_libraries( identityaddvcarddialogtest Qt5::Test Qt5::Widgets KF5::Completion KF5::KIOWidgets KF5::I18n)

set( kmail_statusbarlabeltoggledstate_source statusbarlabeltoggledstatetest.cpp ../widgets/statusbarlabeltoggledstate.cpp ../kmail_debug.cpp)
add_executable(statusbarlabeltoggledstatetest ${kmail_statusbarlabeltoggledstate_source})
ecm_mark_as_test(kmail-statusbarlabeltoggledstatetest)
target_link_libraries( statusbarlabeltoggledstatetest Qt5::Test KF5::KIOWidgets KF5::I18n)


set( kmail_createfollowupreminderonexistingmessagejobtest_source createfollowupreminderonexistingmessagejobtest.cpp ../job/createfollowupreminderonexistingmessagejob.cpp  ../kmail_debug.cpp)
add_executable( createfollowupreminderonexistingmessagejobtest ${kmail_createfollowupreminderonexistingmessagejobtest_source})
ecm_mark_as_test(kmail_createfollowupreminderonexistingmessagejobtest)
target_link_libraries( createfollowupreminderonexistingmessagejobtest Qt5::Test Qt5::Widgets KF5::AkonadiCore KF5::CalendarCore KF5::Mime followupreminder messagecomposer)


set( kmail_cryptostateindicatorwidgettest_source cryptostateindicatorwidgettest.cpp ../editor/widgets/cryptostateindicatorwidget.cpp)
add_executable( cryptostateindicatorwidgettest ${kmail_cryptostateindicatorwidgettest_source})
add_test(cryptostateindicatorwidgettest cryptostateindicatorwidgettest)
ecm_mark_as_test(cryptostateindicatorwidgettest)
target_link_libraries( cryptostateindicatorwidgettest Qt5::Test  messagecore Qt5::Widgets KF5::ConfigWidgets KF5::I18n)


set( kmail_kactionmenutransporttest_source kactionmenutransporttest.cpp ../widgets/kactionmenutransport.cpp)
add_executable( kactionmenutransporttest ${kmail_kactionmenutransporttest_source})
add_test(kactionmenutransporttest kactionmenutransporttest)
ecm_mark_as_test(kactionmenutransporttest)
target_link_libraries( kactionmenutransporttest Qt5::Test  KF5::MailTransport KF5::WidgetsAddons KF5::I18n KF5::ConfigGui)

set( kmail_configureagentlistviewtest_source configureagentlistviewtest.cpp ../configuredialog/configureagents/configureagentitem.cpp
                                            ../configuredialog/configureagents/configureagentlistmodel.cpp ../configuredialog/configureagents/configureagentlistview.cpp
                                            ../configuredialog/configureagents/configureagentlistdelegate.cpp ../kmail_debug.cpp)
add_executable( configureagentlistviewtest ${kmail_configureagentlistviewtest_source})
add_test(configureagentlistviewtest configureagentlistviewtest)
ecm_mark_as_test(configureagentlistviewtest)
target_link_libraries( configureagentlistviewtest Qt5::Test Qt5::Widgets KF5::ItemViews Qt5::DBus)

set( kmail_configureagentitemtest_source configureagentitemtest.cpp ../configuredialog/configureagents/configureagentitem.cpp)
add_executable( configureagentitemtest ${kmail_configureagentitemtest_source})
add_test(configureagentitemtest configureagentitemtest)
ecm_mark_as_test(configureagentitemtest)
target_link_libraries( configureagentitemtest Qt5::Test Qt5::Widgets KF5::ItemViews)

set(KDEPIMLIBS_RUN_ISOLATED_TESTS TRUE)
set(KDEPIMLIBS_RUN_SQLITE_ISOLATED_TESTS TRUE)

add_akonadi_isolated_test_advanced( tagselectdialogtest.cpp  "../tag/tagselectdialog.cpp;../kmail_debug.cpp" "${KDEPIMLIBS_AKONADI_LIBS};kmailprivate;mailcommon;kdepim;KF5::ItemViews")

