/*
  Copyright (c) 2014-2019 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "displaymessageformatactionmenutest.h"
#include "../widgets/displaymessageformatactionmenu.h"
#include <qtest.h>
#include <qtestmouse.h>
#include <QMenu>
#include <KToggleAction>
#include <QSignalSpy>
Q_DECLARE_METATYPE(MessageViewer::Viewer::DisplayFormatMessage)
DisplayMessageFormatActionMenuTest::DisplayMessageFormatActionMenuTest()
{
    qRegisterMetaType<MessageViewer::Viewer::DisplayFormatMessage >();
}

void DisplayMessageFormatActionMenuTest::shouldHaveDefaultValue()
{
    DisplayMessageFormatActionMenu menu;
    QVERIFY(menu.menu());
    KToggleAction *prefereHtml = menu.findChild<KToggleAction *>(QStringLiteral("prefer-html-action"));
    QVERIFY(prefereHtml);
    KToggleAction *prefereText = menu.findChild<KToggleAction *>(QStringLiteral("prefer-text-action"));
    QVERIFY(prefereText);
    KToggleAction *useGlobalSetting = menu.findChild<KToggleAction *>(QStringLiteral("use-global-setting-action"));
    QVERIFY(useGlobalSetting);
    QCOMPARE(useGlobalSetting->isChecked(), true);
    QCOMPARE(menu.menu()->actions().count(), 3);
}

void DisplayMessageFormatActionMenuTest::shouldEmitSignalWhenClickOnSubMenu()
{
    DisplayMessageFormatActionMenu menu;
    KToggleAction *prefereHtml = menu.findChild<KToggleAction *>(QStringLiteral("prefer-html-action"));
    QSignalSpy spy(&menu, SIGNAL(changeDisplayMessageFormat(MessageViewer::Viewer::DisplayFormatMessage)));
    prefereHtml->trigger();
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).value<MessageViewer::Viewer::DisplayFormatMessage>(), MessageViewer::Viewer::Html);
}

void DisplayMessageFormatActionMenuTest::shouldSelectItemWhenChangeFormat()
{
    DisplayMessageFormatActionMenu menu;
    KToggleAction *useGlobalSetting = menu.findChild<KToggleAction *>(QStringLiteral("use-global-setting-action"));
    QCOMPARE(useGlobalSetting->isChecked(), true);
    menu.setDisplayMessageFormat(MessageViewer::Viewer::Text);
    KToggleAction *prefereText = menu.findChild<KToggleAction *>(QStringLiteral("prefer-text-action"));
    QCOMPARE(prefereText->isChecked(), true);
    KToggleAction *prefereHtml = menu.findChild<KToggleAction *>(QStringLiteral("prefer-html-action"));
    QCOMPARE(prefereHtml->isChecked(), false);
    QCOMPARE(useGlobalSetting->isChecked(), false);
}

void DisplayMessageFormatActionMenuTest::shouldDontEmitSignalWhenChangeFormat()
{
    DisplayMessageFormatActionMenu menu;
    QSignalSpy spy(&menu, SIGNAL(changeDisplayMessageFormat(MessageViewer::Viewer::DisplayFormatMessage)));
    menu.setDisplayMessageFormat(MessageViewer::Viewer::Text);
    QCOMPARE(spy.count(), 0);
}

QTEST_MAIN(DisplayMessageFormatActionMenuTest)
