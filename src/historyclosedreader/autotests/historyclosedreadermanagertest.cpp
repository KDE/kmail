/*
    SPDX-FileCopyrightText: 2023 Laurent Montel <montel@kde.org>
    SPDX-License-Identifier: GPL-2.0-only
*/

#include "historyclosedreadermanagertest.h"
#include "historyclosedreader/historyclosedreadermanager.h"
#include <QTest>
QTEST_GUILESS_MAIN(HistoryClosedReaderManagerTest)
HistoryClosedReaderManagerTest::HistoryClosedReaderManagerTest(QObject *parent)
    : QObject{parent}
{
}

void HistoryClosedReaderManagerTest::shouldHaveDefaultValues()
{
    HistoryClosedReaderManager w;
    QVERIFY(w.isEmpty());
}

void HistoryClosedReaderManagerTest::shouldAddValues()
{
    HistoryClosedReaderManager w;
    {
        HistoryClosedReaderInfo info;
        info.setItem(2);
        info.setSubject(QStringLiteral("sub"));
        w.addInfo(std::move(info));
    }
    QVERIFY(!w.isEmpty());
    QCOMPARE(w.count(), 1);
    {
        HistoryClosedReaderInfo info;
        info.setItem(5);
        info.setSubject(QStringLiteral("sub2"));
        w.addInfo(std::move(info));
    }
    QCOMPARE(w.count(), 2);
}

void HistoryClosedReaderManagerTest::shouldClear()
{
    HistoryClosedReaderManager w;
    {
        HistoryClosedReaderInfo info;
        info.setItem(2);
        info.setSubject(QStringLiteral("sub"));
        w.addInfo(std::move(info));
    }
    QVERIFY(!w.isEmpty());
    {
        HistoryClosedReaderInfo info;
        info.setItem(5);
        info.setSubject(QStringLiteral("sub2"));
        w.addInfo(std::move(info));
    }
    QCOMPARE(w.count(), 2);
    w.clear();
    QVERIFY(w.isEmpty());
}

#include "moc_historyclosedreadermanagertest.cpp"
