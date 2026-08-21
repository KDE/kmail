/*
  SPDX-FileCopyrightText: 2015-2026 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kactionmenutransporttest.h"
#include "../widgets/kactionmenutransport.h"
#include <QMenu>
#include <QStandardPaths>
#include <QTest>
QTEST_MAIN(KActionMenuTransportTest)
KActionMenuTransportTest::KActionMenuTransportTest(QObject *parent)
    : QObject(parent)
{
    QStandardPaths::setTestModeEnabled(true);
}

KActionMenuTransportTest::~KActionMenuTransportTest() = default;

void KActionMenuTransportTest::shouldHaveDefaultValue()
{
    KActionMenuTransport w;
    QVERIFY(w.menu());
    QCOMPARE(w.menu()->actions().count(), 0);
}

#include "moc_kactionmenutransporttest.cpp"
