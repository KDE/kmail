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

#include "moc_historyclosedreadermanagertest.cpp"
