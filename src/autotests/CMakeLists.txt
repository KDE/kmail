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
target_link_libraries( displaymessageformatactionmenutest Qt::Test  KPim${KF_MAJOR_VERSION}::MessageViewer KF${KF_MAJOR_VERSION}::I18n kmailprivate)

#####
add_executable( identityaddvcarddialogtest  identityaddvcarddialogtest.cpp identityaddvcarddialogtest.h)
ecm_mark_as_test(kmail-identityaddvcarddialogtest)
target_link_libraries( identityaddvcarddialogtest Qt::Test Qt::Widgets KF${KF_MAJOR_VERSION}::Completion KF${KF_MAJOR_VERSION}::KIOWidgets KF${KF_MAJOR_VERSION}::I18n kmailprivate)

#####
add_executable( createfollowupreminderonexistingmessagejobtest createfollowupreminderonexistingmessagejobtest.cpp createfollowupreminderonexistingmessagejobtest.h)
ecm_mark_as_test(kmail_createfollowupreminderonexistingmessagejobtest)
target_link_libraries( createfollowupreminderonexistingmessagejobtest Qt::Test Qt::Widgets KPim${KF_MAJOR_VERSION}::AkonadiCore KF${KF_MAJOR_VERSION}::CalendarCore KPim${KF_MAJOR_VERSION}::Mime KPim${KF_MAJOR_VERSION}::MessageComposer kmailprivate)
#####

add_executable( cryptostateindicatorwidgettest cryptostateindicatorwidgettest.cpp cryptostateindicatorwidgettest.h)
add_test(NAME cryptostateindicatorwidgettest COMMAND cryptostateindicatorwidgettest)
ecm_mark_as_test(cryptostateindicatorwidgettest)
target_link_libraries( cryptostateindicatorwidgettest Qt::Test  KPim${KF_MAJOR_VERSION}::MessageCore Qt::Widgets KF${KF_MAJOR_VERSION}::ConfigWidgets KF${KF_MAJOR_VERSION}::I18n kmailprivate)
#####

add_executable( encryptionstatetest encryptionstatetest.cpp encryptionstatetest.h)
add_test(NAME encryptionstatetest COMMAND encryptionstatetest)
ecm_mark_as_test(encryptionstatetest)
target_link_libraries( encryptionstatetest Qt::Test kmailprivate)

add_executable( kactionmenutransporttest kactionmenutransporttest.cpp kactionmenutransporttest.h)
add_test(NAME kactionmenutransporttest COMMAND kactionmenutransporttest)
ecm_mark_as_test(kactionmenutransporttest)
target_link_libraries( kactionmenutransporttest Qt::Test  KPim${KF_MAJOR_VERSION}::AkonadiMime KF${KF_MAJOR_VERSION}::WidgetsAddons KF${KF_MAJOR_VERSION}::I18n KF${KF_MAJOR_VERSION}::ConfigGui kmailprivate)

add_akonadi_isolated_test_advanced( kmcomposerwintest.cpp "kmcomposerwintest.h"
   "Qt::Test;KPim${KF_MAJOR_VERSION}::IdentityManagementWidgets;KPim${KF_MAJOR_VERSION}::Libkleo;KPim${KF_MAJOR_VERSION}::MessageCore;KPim${KF_MAJOR_VERSION}::MessageComposer;KPim${KF_MAJOR_VERSION}::TemplateParser;KF${KF_MAJOR_VERSION}::XmlGui;Qt::Widgets;KF${KF_MAJOR_VERSION}::ConfigWidgets;KF${KF_MAJOR_VERSION}::I18n;kmailprivate")

add_akonadi_isolated_test_advanced( tagselectdialogtest.cpp  "tagselectdialogtest.h" "kmailprivate;KPim${KF_MAJOR_VERSION}::MailCommon;KPim${KF_MAJOR_VERSION}::Libkdepim;KF${KF_MAJOR_VERSION}::ItemViews;KPim${KF_MAJOR_VERSION}::TemplateParser;KF${KF_MAJOR_VERSION}::XmlGui;KF${KF_MAJOR_VERSION}::Completion;KF${KF_MAJOR_VERSION}::I18n")

add_akonadi_isolated_test_advanced(kmcommandstest.cpp "kmcommandstest.h"
    "Qt::Test;Qt::Widgets;KPim${KF_MAJOR_VERSION}::AkonadiCore;KF${KF_MAJOR_VERSION}::XmlGui;KF${KF_MAJOR_VERSION}::Bookmarks;KF${KF_MAJOR_VERSION}::ConfigWidgets;KF${KF_MAJOR_VERSION}::Contacts;KF${KF_MAJOR_VERSION}::I18n;KPim${KF_MAJOR_VERSION}::IdentityManagement;KF${KF_MAJOR_VERSION}::KIOCore;KF${KF_MAJOR_VERSION}::KIOFileWidgets;KPim${KF_MAJOR_VERSION}::MessageCore;KPim${KF_MAJOR_VERSION}::MessageComposer;KPim${KF_MAJOR_VERSION}::MessageList;KPim${KF_MAJOR_VERSION}::MessageViewer;KPim${KF_MAJOR_VERSION}::MailCommon;KPim${KF_MAJOR_VERSION}::AkonadiMime;KPim${KF_MAJOR_VERSION}::Libkdepim;KPim${KF_MAJOR_VERSION}::TemplateParser;kmailprivate")
