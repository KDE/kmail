/*
  SPDX-FileCopyrightText: 2014-2026 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "displaymessageformatactionmenutest.h"
#include "../widgets/displaymessageformatactionmenu.h"
#include <KToggleAction>
#include <QMenu>
#include <QSignalSpy>
#include <QTest>
#include <qtestmouse.h>
using namespace Qt::Literals::StringLiterals;

Q_DECLARE_METATYPE(MessageViewer::Viewer::DisplayFormatMessage)
QTEST_MAIN(DisplayMessageFormatActionMenuTest)
DisplayMessageFormatActionMenuTest::DisplayMessageFormatActionMenuTest(QObject *parent)
    : QObject(parent)
{
    qRegisterMetaType<MessageViewer::Viewer::DisplayFormatMessage>();
}

void DisplayMessageFormatActionMenuTest::shouldHaveDefaultValue()
{
    DisplayMessageFormatActionMenu menu;
    QVERIFY(menu.menu());
    auto prefereHtml = menu.findChild<KToggleAction *>(u"prefer-html-action"_s);
    QVERIFY(prefereHtml);
    auto prefereText = menu.findChild<KToggleAction *>(u"prefer-text-action"_s);
    QVERIFY(prefereText);
    auto useGlobalSetting = menu.findChild<KToggleAction *>(u"use-global-setting-action"_s);
    QVERIFY(useGlobalSetting);
    QCOMPARE(useGlobalSetting->isChecked(), true);
    QCOMPARE(menu.menu()->actions().count(), 3);
}

void DisplayMessageFormatActionMenuTest::shouldEmitSignalWhenClickOnSubMenu()
{
    DisplayMessageFormatActionMenu menu;
    auto prefereHtml = menu.findChild<KToggleAction *>(u"prefer-html-action"_s);
    QSignalSpy spy(&menu, &DisplayMessageFormatActionMenu::changeDisplayMessageFormat);
    prefereHtml->trigger();
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).value<MessageViewer::Viewer::DisplayFormatMessage>(), MessageViewer::Viewer::Html);
}

void DisplayMessageFormatActionMenuTest::shouldSelectItemWhenChangeFormat()
{
    DisplayMessageFormatActionMenu menu;
    auto useGlobalSetting = menu.findChild<KToggleAction *>(u"use-global-setting-action"_s);
    QCOMPARE(useGlobalSetting->isChecked(), true);
    menu.setDisplayMessageFormat(MessageViewer::Viewer::Text);
    auto prefereText = menu.findChild<KToggleAction *>(u"prefer-text-action"_s);
    QCOMPARE(prefereText->isChecked(), true);
    auto prefereHtml = menu.findChild<KToggleAction *>(u"prefer-html-action"_s);
    QCOMPARE(prefereHtml->isChecked(), false);
    QCOMPARE(useGlobalSetting->isChecked(), false);
}

void DisplayMessageFormatActionMenuTest::shouldDontEmitSignalWhenChangeFormat()
{
    DisplayMessageFormatActionMenu menu;
    QSignalSpy spy(&menu, &DisplayMessageFormatActionMenu::changeDisplayMessageFormat);
    menu.setDisplayMessageFormat(MessageViewer::Viewer::Text);
    QCOMPARE(spy.count(), 0);
}

#include "moc_displaymessageformatactionmenutest.cpp"
