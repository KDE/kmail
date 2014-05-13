/*
  Copyright (c) 2014 Montel Laurent <montel@kde.org>

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
#include <qtest_kde.h>
#include <qtestmouse.h>
#include <KMenu>
#include <KToggleAction>
Q_DECLARE_METATYPE(MessageViewer::Viewer::DisplayFormatMessage)
DisplayMessageFormatActionMenuTest::DisplayMessageFormatActionMenuTest()
{
    qRegisterMetaType<MessageViewer::Viewer::DisplayFormatMessage >();
}

void DisplayMessageFormatActionMenuTest::shouldHaveDefaultValue()
{
    DisplayMessageFormatActionMenu menu;
    QVERIFY(menu.menu());
    KToggleAction *prefereHtml = qFindChild<KToggleAction *>(&menu, QLatin1String("prefer-html-action"));
    QVERIFY(prefereHtml);
    KToggleAction *prefereText = qFindChild<KToggleAction *>(&menu, QLatin1String("prefer-text-action"));
    QVERIFY(prefereText);
    KToggleAction *useGlobalSetting = qFindChild<KToggleAction *>(&menu, QLatin1String("use-global-setting-action"));
    QVERIFY(useGlobalSetting);
    QCOMPARE(useGlobalSetting->isChecked(), true);
    QCOMPARE(menu.menu()->actions().count(), 3);
}

void DisplayMessageFormatActionMenuTest::shouldEmitSignalWhenClickOnSubMenu()
{
    DisplayMessageFormatActionMenu menu;
    KToggleAction *prefereHtml = qFindChild<KToggleAction *>(&menu, QLatin1String("prefer-html-action"));
    QSignalSpy spy(&menu, SIGNAL(changeDisplayMessageFormat(MessageViewer::Viewer::DisplayFormatMessage)));
    prefereHtml->trigger();
    QCOMPARE(spy.count(), 1);
}

QTEST_KDEMAIN(DisplayMessageFormatActionMenuTest, GUI)
