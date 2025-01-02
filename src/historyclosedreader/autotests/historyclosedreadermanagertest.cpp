/*
    SPDX-FileCopyrightText: 2023-2025 Laurent Montel <montel@kde.org>
    SPDX-License-Identifier: GPL-2.0-only
*/

#include "historyclosedreadermanagertest.h"
#include "historyclosedreader/historyclosedreadermanager.h"
#include <QSignalSpy>
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
    QVERIFY(w.closedReaderInfos().isEmpty());
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

void HistoryClosedReaderManagerTest::shouldRemoveValue()
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
    // Don't compare value
    (void)w.lastInfo();
    QCOMPARE(w.count(), 1);
    (void)w.lastInfo();
    QCOMPARE(w.count(), 0);
    QVERIFY(w.isEmpty());
}

void HistoryClosedReaderManagerTest::shouldEmitSignal()
{
    HistoryClosedReaderManager w;
    QSignalSpy spyHistoryClosedReaderChanged(&w, &HistoryClosedReaderManager::historyClosedReaderChanged);
    // List is empty => clearing will not emit signal.
    w.clear();
    QCOMPARE(spyHistoryClosedReaderChanged.count(), 0);

    {
        HistoryClosedReaderInfo info;
        info.setItem(2);
        info.setSubject(QStringLiteral("sub"));
        w.addInfo(std::move(info));
        QCOMPARE(spyHistoryClosedReaderChanged.count(), 1);
        w.clear();
        QCOMPARE(spyHistoryClosedReaderChanged.count(), 2);
    }
    w.clear();
    spyHistoryClosedReaderChanged.clear();

    {
        HistoryClosedReaderInfo info;
        info.setItem(2);
        info.setSubject(QStringLiteral("sub"));
        w.addInfo(std::move(info));

        HistoryClosedReaderInfo info2;
        info2.setItem(3);
        info2.setSubject(QStringLiteral("sub2"));
        w.addInfo(std::move(info2));
        spyHistoryClosedReaderChanged.clear();

        (void)w.lastInfo();
        QCOMPARE(spyHistoryClosedReaderChanged.count(), 0);

        (void)w.lastInfo();
        QCOMPARE(spyHistoryClosedReaderChanged.count(), 1);
    }
    // TODO
}

void HistoryClosedReaderManagerTest::shouldAssignMaxValues()
{
    HistoryClosedReaderManager w;
    for (int i = 0; i < 15; i++) {
        HistoryClosedReaderInfo info;
        info.setItem(2 + i);
        info.setSubject(QStringLiteral("sub"));
        w.addInfo(std::move(info));
    }
    QCOMPARE(w.count(), 10);
}

#include "moc_historyclosedreadermanagertest.cpp"
