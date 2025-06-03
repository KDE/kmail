/*
  SPDX-FileCopyrightText: 2015-2025 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kactionmenutransporttest.h"
#include "../widgets/kactionmenutransport.h"
#include <QMenu>
#include <QTest>
KActionMenuTransportTest::KActionMenuTransportTest(QObject *parent)
    : QObject(parent)
{
}

KActionMenuTransportTest::~KActionMenuTransportTest() = default;

void KActionMenuTransportTest::shouldHaveDefaultValue()
{
    KActionMenuTransport w;
    QVERIFY(w.menu());
    QCOMPARE(w.menu()->actions().count(), 0);
}

QTEST_MAIN(KActionMenuTransportTest)

#include "moc_kactionmenutransporttest.cpp"
