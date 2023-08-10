/*
   SPDX-FileCopyrightText: 2012-2023 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "archivemailagentutiltest.h"
#include "archivemailagentutil.h"
#include <QTest>
QTEST_GUILESS_MAIN(ArchiveMailAgentUtilTest)
ArchiveMailAgentUtilTest::ArchiveMailAgentUtilTest(QObject *parent)
    : QObject{parent}
{
}

void ArchiveMailAgentUtilTest::shouldTestTimeIsInRange()
{
    QFETCH(QList<int>, range);
    QFETCH(QTime, time);
    QFETCH(bool, result);
    QCOMPARE(ArchiveMailAgentUtil::timeIsInRange(range, time), result);
}

void ArchiveMailAgentUtilTest::shouldTestTimeIsInRange_data()
{
    QTest::addColumn<QList<int>>("range");
    QTest::addColumn<QTime>("time");
    QTest::addColumn<bool>("result");

    // QTest::newRow("first") << 0 << 0;
}
