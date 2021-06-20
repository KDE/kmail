set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
add_definitions( -DTEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data" )
include_directories(
    BEFORE
    ${kmail_SOURCE_DIR}
    ${kmail_BINARY_DIR}
    )

########### dbus test ###############
add_executable( displaymessageformatactionmenutest displaymessageformatactionmenutest.cpp )
add_test(NAME displaymessageformatactionmenutest COMMAND displaymessageformatactionmenutest )
ecm_mark_as_test(kmail-displaymessageformatactionmenutest)
target_link_libraries( displaymessageformatactionmenutest Qt::Test  KF5::MessageViewer KF5::I18n kmailprivate)

#####
add_executable( identityaddvcarddialogtest  identityaddvcarddialogtest.cpp)
ecm_mark_as_test(kmail-identityaddvcarddialogtest)
target_link_libraries( identityaddvcarddialogtest Qt::Test Qt::Widgets KF5::Completion KF5::KIOWidgets KF5::I18n kmailprivate)

#####
add_executable( createfollowupreminderonexistingmessagejobtest createfollowupreminderonexistingmessagejobtest.cpp)
ecm_mark_as_test(kmail_createfollowupreminderonexistingmessagejobtest)
target_link_libraries( createfollowupreminderonexistingmessagejobtest Qt::Test Qt::Widgets KF5::AkonadiCore KF5::CalendarCore KF5::Mime KF5::MessageComposer kmailprivate)
#####

add_executable( cryptostateindicatorwidgettest cryptostateindicatorwidgettest.cpp)
add_test(NAME cryptostateindicatorwidgettest COMMAND cryptostateindicatorwidgettest)
ecm_mark_as_test(cryptostateindicatorwidgettest)
target_link_libraries( cryptostateindicatorwidgettest Qt::Test  KF5::MessageCore Qt::Widgets KF5::ConfigWidgets KF5::I18n kmailprivate)
#####

add_executable( kactionmenutransporttest kactionmenutransporttest.cpp)
add_test(NAME kactionmenutransporttest COMMAND kactionmenutransporttest)
ecm_mark_as_test(kactionmenutransporttest)
target_link_libraries( kactionmenutransporttest Qt::Test  KF5::MailTransportAkonadi KF5::WidgetsAddons KF5::I18n KF5::ConfigGui kmailprivate)

if (KDEPIM_RUN_AKONADI_TEST)
    set(KDEPIMLIBS_RUN_ISOLATED_TESTS TRUE)
    set(KDEPIMLIBS_RUN_SQLITE_ISOLATED_TESTS TRUE)

    add_akonadi_isolated_test_advanced( kmcomposerwintest.cpp ""
    "Qt::Test;KF5::IdentityManagement;KF5::MessageCore;KF5::TemplateParser;KF5::XmlGui;Qt::Widgets;KF5::ConfigWidgets;KF5::I18n;kmailprivate")

    add_akonadi_isolated_test_advanced( tagselectdialogtest.cpp  "" "kmailprivate;KF5::MailCommon;KF5::Libkdepim;KF5::ItemViews;KF5::TemplateParser;KF5::XmlGui;KF5::Completion;KF5::I18n")

    add_akonadi_isolated_test_advanced(kmcommandstest.cpp ""
	"Qt::Test;Qt::Widgets;KF5::AkonadiCore;KF5::Bookmarks;KF5::ConfigWidgets;KF5::Contacts;KF5::I18n;KF5::IdentityManagement;KF5::KIOCore;KF5::KIOFileWidgets;KF5::MessageCore;KF5::MessageComposer;KF5::MessageList;KF5::MessageViewer;KF5::MailCommon;KF5::MailTransportAkonadi;KF5::Libkdepim;KF5::TemplateParser;kmailprivate")
endif()
