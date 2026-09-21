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
#include "moc_snoozeattributetest.cpp"
