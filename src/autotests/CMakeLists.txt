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
target_link_libraries( displaymessageformatactionmenutest Qt::Test  KF5::MessageViewer KF${KF_MAJOR_VERSION}::I18n kmailprivate)

#####
add_executable( identityaddvcarddialogtest  identityaddvcarddialogtest.cpp identityaddvcarddialogtest.h)
ecm_mark_as_test(kmail-identityaddvcarddialogtest)
target_link_libraries( identityaddvcarddialogtest Qt::Test Qt::Widgets KF${KF_MAJOR_VERSION}::Completion KF${KF_MAJOR_VERSION}::KIOWidgets KF${KF_MAJOR_VERSION}::I18n kmailprivate)

#####
add_executable( createfollowupreminderonexistingmessagejobtest createfollowupreminderonexistingmessagejobtest.cpp createfollowupreminderonexistingmessagejobtest.h)
ecm_mark_as_test(kmail_createfollowupreminderonexistingmessagejobtest)
target_link_libraries( createfollowupreminderonexistingmessagejobtest Qt::Test Qt::Widgets KF5::AkonadiCore KF${KF_MAJOR_VERSION}::CalendarCore KPim${KF_MAJOR_VERSION}::Mime KF5::MessageComposer kmailprivate)
#####

add_executable( cryptostateindicatorwidgettest cryptostateindicatorwidgettest.cpp cryptostateindicatorwidgettest.h)
add_test(NAME cryptostateindicatorwidgettest COMMAND cryptostateindicatorwidgettest)
ecm_mark_as_test(cryptostateindicatorwidgettest)
target_link_libraries( cryptostateindicatorwidgettest Qt::Test  KF5::MessageCore Qt::Widgets KF${KF_MAJOR_VERSION}::ConfigWidgets KF${KF_MAJOR_VERSION}::I18n kmailprivate)
#####

add_executable( kactionmenutransporttest kactionmenutransporttest.cpp kactionmenutransporttest.h)
add_test(NAME kactionmenutransporttest COMMAND kactionmenutransporttest)
ecm_mark_as_test(kactionmenutransporttest)
target_link_libraries( kactionmenutransporttest Qt::Test  KF5::MailTransportAkonadi KF${KF_MAJOR_VERSION}::WidgetsAddons KF${KF_MAJOR_VERSION}::I18n KF${KF_MAJOR_VERSION}::ConfigGui kmailprivate)


add_akonadi_isolated_test_advanced( kmcomposerwintest.cpp "kmcomposerwintest.h"
    "Qt::Test;KF5::IdentityManagementWidgets;KF5::MessageCore;KF5::TemplateParser;KF${KF_MAJOR_VERSION}::XmlGui;Qt::Widgets;KF${KF_MAJOR_VERSION}::ConfigWidgets;KF${KF_MAJOR_VERSION}::I18n;kmailprivate")

add_akonadi_isolated_test_advanced( tagselectdialogtest.cpp  "tagselectdialogtest.h" "kmailprivate;KF5::MailCommon;KPim${KF_MAJOR_VERSION}::Libkdepim;KF${KF_MAJOR_VERSION}::ItemViews;KF5::TemplateParser;KF${KF_MAJOR_VERSION}::XmlGui;KF${KF_MAJOR_VERSION}::Completion;KF${KF_MAJOR_VERSION}::I18n")

add_akonadi_isolated_test_advanced(kmcommandstest.cpp "kmcommandstest.h"
    "Qt::Test;Qt::Widgets;KF5::AkonadiCore;KF${KF_MAJOR_VERSION}::XmlGui;KF${KF_MAJOR_VERSION}::Bookmarks;KF${KF_MAJOR_VERSION}::ConfigWidgets;KF${KF_MAJOR_VERSION}::Contacts;KF${KF_MAJOR_VERSION}::I18n;KPim${KF_MAJOR_VERSION}::IdentityManagement;KF${KF_MAJOR_VERSION}::KIOCore;KF${KF_MAJOR_VERSION}::KIOFileWidgets;KF5::MessageCore;KF5::MessageComposer;KF5::MessageList;KF5::MessageViewer;KF5::MailCommon;KF5::MailTransportAkonadi;KPim${KF_MAJOR_VERSION}::Libkdepim;KF5::TemplateParser;kmailprivate")
