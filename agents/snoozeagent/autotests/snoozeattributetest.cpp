/*
   SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "snoozeattributetest.h"
#include "snoozeattribute.h"
#include <QTest>
QTEST_GUILESS_MAIN(SnoozeAttributeTest)
SnoozeAttributeTest::SnoozeAttributeTest(QObject *parent)
    : QObject{parent}
{
}

void SnoozeAttributeTest::shouldHaveDefaultValues()
{
    const SnoozeAttribute attr;
    QVERIFY(!attr.wakeUpDateTime().isValid());
}

void SnoozeAttributeTest::shouldCloneAttributes()
{
    SnoozeAttribute attr;
    attr.setWakeUpDateTime(QDateTime(QDate(2026, 9, 9), QTime(8, 8, 8)));

    SnoozeAttribute *result = attr.clone();
    QCOMPARE(attr, *result);
    delete result;
}

#include "moc_snoozeattributetest.cpp"
