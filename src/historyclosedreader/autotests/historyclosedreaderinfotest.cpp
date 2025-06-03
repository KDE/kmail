/*
    SPDX-FileCopyrightText: 2023-2025 Laurent Montel <montel@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "historyclosedreaderinfotest.h"
#include "historyclosedreader/historyclosedreaderinfo.h"
#include <Akonadi/Item>
#include <QTest>
QTEST_GUILESS_MAIN(HistoryClosedReaderInfoTest)
HistoryClosedReaderInfoTest::HistoryClosedReaderInfoTest(QObject *parent)
    : QObject{parent}
{
}

void HistoryClosedReaderInfoTest::shouldHaveDefaultValues()
{
    HistoryClosedReaderInfo w;
    QVERIFY(w.subject().isEmpty());
    QCOMPARE(w.item(), -1);
}

#include "moc_historyclosedreaderinfotest.cpp"
