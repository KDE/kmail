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

    {
        // 5h30m1
        const QTime t(5, 30, 1);
        // between 4h and 8h
        const QList<int> interval{4, 8};
        QTest::newRow("test1") << interval << t << true;
    }
    {
        // 5h30m1
        const QTime t(5, 30, 1);
        // between 12h and 15h
        const QList<int> interval{12, 15};
        QTest::newRow("test2") << interval << t << false;
    }
    {
        // 5h30m1
        const QTime t(5, 30, 1);
        // between 5h and 6h
        const QList<int> interval{5, 6};
        QTest::newRow("test3") << interval << t << true;
    }
    {
        // 23h30m1
        const QTime t(23, 30, 1);
        // between 22h and 4h
        const QList<int> interval{22, 4};
        QTest::newRow("test4") << interval << t << true;
    }
    {
        // 20h30m1
        const QTime t(20, 30, 1);
        // between 22h and 4h
        const QList<int> interval{22, 4};
        QTest::newRow("test5") << interval << t << false;
    }
    {
        // 1h30m1
        const QTime t(1, 30, 1);
        // between 22h and 4h
        const QList<int> interval{22, 4};
        QTest::newRow("test6") << interval << t << true;
    }
    {
        // 5h30m1
        const QTime t(5, 30, 1);
        // between 22h and 4h
        const QList<int> interval{22, 4};
        QTest::newRow("test7") << interval << t << false;
    }
    {
        // 5h30m1
        const QTime t(5, 30, 1);
        // between 22h and 21h
        const QList<int> interval{22, 21};
        QTest::newRow("test8") << interval << t << true;
    }
    {
        // 21h30m1
        const QTime t(21, 30, 1);
        // between 22h and 21h
        const QList<int> interval{22, 20};
        QTest::newRow("test9") << interval << t << false;
    }
}
