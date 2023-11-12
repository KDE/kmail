# SPDX-FileCopyrightText: none
# SPDX-License-Identifier: BSD-3-Clause
set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
add_definitions( -DTEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data" )
include_directories(
    BEFORE
    ${kmail_SOURCE_DIR}
    ${kmail_BINARY_DIR}
    )

########### dbus test ###############
add_executable( displaymessageformatactionmenutest displaymessageformatactionmenutest.cpp displaymessageformatactionmenutest.h)
add_test(NAME displaymessageformatactionmenutest COMMAND displaymessageformatactionmenutest )
ecm_mark_as_test(kmail-displaymessageformatactionmenutest)
target_link_libraries( displaymessageformatactionmenutest Qt::Test  KPim6::MessageViewer KF6::I18n kmailprivate)

#####
add_executable( identityaddvcarddialogtest  identityaddvcarddialogtest.cpp identityaddvcarddialogtest.h)
ecm_mark_as_test(kmail-identityaddvcarddialogtest)
target_link_libraries( identityaddvcarddialogtest Qt::Test Qt::Widgets KF6::Completion KF6::KIOWidgets KF6::I18n kmailprivate)

#####
add_executable( createfollowupreminderonexistingmessagejobtest createfollowupreminderonexistingmessagejobtest.cpp createfollowupreminderonexistingmessagejobtest.h)
ecm_mark_as_test(kmail_createfollowupreminderonexistingmessagejobtest)
target_link_libraries( createfollowupreminderonexistingmessagejobtest Qt::Test Qt::Widgets KPim6::AkonadiCore KF6::CalendarCore KPim6::Mime KPim6::MessageComposer kmailprivate)
#####

add_executable( cryptostateindicatorwidgettest cryptostateindicatorwidgettest.cpp cryptostateindicatorwidgettest.h)
add_test(NAME cryptostateindicatorwidgettest COMMAND cryptostateindicatorwidgettest)
ecm_mark_as_test(cryptostateindicatorwidgettest)
target_link_libraries( cryptostateindicatorwidgettest Qt::Test  KPim6::MessageCore Qt::Widgets KF6::ConfigWidgets KF6::I18n kmailprivate)
#####

add_executable( encryptionstatetest encryptionstatetest.cpp encryptionstatetest.h)
add_test(NAME encryptionstatetest COMMAND encryptionstatetest)
ecm_mark_as_test(encryptionstatetest)
target_link_libraries( encryptionstatetest Qt::Test kmailprivate)

add_executable( kactionmenutransporttest kactionmenutransporttest.cpp kactionmenutransporttest.h)
add_test(NAME kactionmenutransporttest COMMAND kactionmenutransporttest)
ecm_mark_as_test(kactionmenutransporttest)
target_link_libraries( kactionmenutransporttest Qt::Test  KPim6::AkonadiMime KF6::WidgetsAddons KF6::I18n KF6::ConfigGui kmailprivate)

add_akonadi_isolated_test_advanced( kmcomposerwintest.cpp "kmcomposerwintest.h"
	"Qt::Test;KPim6::IdentityManagementWidgets;KPim6::Libkleo;KPim6::MessageCore;KPim6::MessageComposer;KPim6::TemplateParser;KF6::XmlGui;Qt::Widgets;KF6::ConfigWidgets;KF6::I18n;kmailprivate;KF6::Codecs")

add_akonadi_isolated_test_advanced( tagselectdialogtest.cpp  "tagselectdialogtest.h" "kmailprivate;KPim6::MailCommon;KPim6::Libkdepim;KF6::ItemViews;KPim6::TemplateParser;KF6::XmlGui;KF6::Completion;KF6::I18n")

add_akonadi_isolated_test_advanced(kmcommandstest.cpp "kmcommandstest.h"
    "Qt::Test;Qt::Widgets;KPim6::AkonadiCore;KF6::XmlGui;KF6::Bookmarks;KF6::ConfigWidgets;KF6::Contacts;KF6::I18n;KPim6::IdentityManagementCore;KF6::KIOCore;KF6::KIOFileWidgets;KPim6::MessageCore;KPim6::MessageComposer;KPim6::MessageList;KPim6::MessageViewer;KPim6::MailCommon;KPim6::AkonadiMime;KPim6::Libkdepim;KPim6::TemplateParser;kmailprivate")


add_executable( commandlineinfotest commandlineinfotest.cpp commandlineinfotest.h)
add_test(NAME commandlineinfotest COMMAND commandlineinfotest)
ecm_mark_as_test(commandlineinfotest)
target_link_libraries( commandlineinfotest Qt::Test  KPim6::AkonadiMime KF6::WidgetsAddons KF6::I18n KF6::ConfigGui kmailprivate)
