/*
  SPDX-FileCopyrightText: 2014-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#include "displaymessageformatactionmenutest.h"
#include "../widgets/displaymessageformatactionmenu.h"
#include <KToggleAction>
#include <QMenu>
#include <QSignalSpy>
#include <QTest>
#include <qtestmouse.h>
Q_DECLARE_METATYPE(MessageViewer::Viewer::DisplayFormatMessage)
DisplayMessageFormatActionMenuTest::DisplayMessageFormatActionMenuTest(QObject *parent)
    : QObject(parent)
{
    qRegisterMetaType<MessageViewer::Viewer::DisplayFormatMessage>();
}

void DisplayMessageFormatActionMenuTest::shouldHaveDefaultValue()
{
    DisplayMessageFormatActionMenu menu;
    QVERIFY(menu.menu());
    auto prefereHtml = menu.findChild<KToggleAction *>(QStringLiteral("prefer-html-action"));
    QVERIFY(prefereHtml);
    auto prefereText = menu.findChild<KToggleAction *>(QStringLiteral("prefer-text-action"));
    QVERIFY(prefereText);
    auto useGlobalSetting = menu.findChild<KToggleAction *>(QStringLiteral("use-global-setting-action"));
    QVERIFY(useGlobalSetting);
    QCOMPARE(useGlobalSetting->isChecked(), true);
    QCOMPARE(menu.menu()->actions().count(), 3);
}

void DisplayMessageFormatActionMenuTest::shouldEmitSignalWhenClickOnSubMenu()
{
    DisplayMessageFormatActionMenu menu;
    auto prefereHtml = menu.findChild<KToggleAction *>(QStringLiteral("prefer-html-action"));
    QSignalSpy spy(&menu, &DisplayMessageFormatActionMenu::changeDisplayMessageFormat);
    prefereHtml->trigger();
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).value<MessageViewer::Viewer::DisplayFormatMessage>(), MessageViewer::Viewer::Html);
}

void DisplayMessageFormatActionMenuTest::shouldSelectItemWhenChangeFormat()
{
    DisplayMessageFormatActionMenu menu;
    auto useGlobalSetting = menu.findChild<KToggleAction *>(QStringLiteral("use-global-setting-action"));
    QCOMPARE(useGlobalSetting->isChecked(), true);
    menu.setDisplayMessageFormat(MessageViewer::Viewer::Text);
    auto prefereText = menu.findChild<KToggleAction *>(QStringLiteral("prefer-text-action"));
    QCOMPARE(prefereText->isChecked(), true);
    auto prefereHtml = menu.findChild<KToggleAction *>(QStringLiteral("prefer-html-action"));
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

QTEST_MAIN(DisplayMessageFormatActionMenuTest)
