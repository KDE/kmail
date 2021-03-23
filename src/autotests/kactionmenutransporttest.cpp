/*
  SPDX-FileCopyrightText: 2015-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
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
