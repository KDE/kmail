/*
   SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "snoozeinfotest.h"
#include "snoozeinfo.h"

#include <QTest>
QTEST_GUILESS_MAIN(SnoozeInfoTest)
SnoozeInfoTest::SnoozeInfoTest(QObject *parent)
    : QObject(parent)
{
}

void SnoozeInfoTest::shouldHaveDefaultValue()
{
    Snooze::SnoozeInfo info;
    QCOMPARE(info.itemId(), -1);
    QCOMPARE(info.originalCollection(), -1);
    QCOMPARE(info.uniqueIdentifier(), -1);
    QVERIFY(!info.wakeUpDateTime().isValid());
    QVERIFY(info.subject().isEmpty());
    QVERIFY(info.from().isEmpty());
    QVERIFY(info.markAsUnread());
    QVERIFY(!info.isValid());
}

void SnoozeInfoTest::shouldBeInvalidWithoutWakeUpDate()
{
    Snooze::SnoozeInfo info;
    info.setItemId(42);
    QVERIFY(!info.isValid());
}

void SnoozeInfoTest::shouldRestoreFromConfig()
{
    // TODO write to a KConfigGroup and read it back, once writeConfig() is implemented.
}

#include "moc_snoozeinfotest.cpp"
