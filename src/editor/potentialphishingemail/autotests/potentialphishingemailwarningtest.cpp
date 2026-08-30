/*
  SPDX-FileCopyrightText: 2015-2026 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later

*/

#include "potentialphishingemailwarningtest.h"
#include "../potentialphishingemailwarning.h"
#include <QAction>
#include <QTest>
using namespace Qt::Literals::StringLiterals;

PotentialPhishingEmailWarningTest::PotentialPhishingEmailWarningTest(QObject *parent)
    : QObject(parent)
{
}

PotentialPhishingEmailWarningTest::~PotentialPhishingEmailWarningTest() = default;

void PotentialPhishingEmailWarningTest::shouldHaveDefaultValue()
{
    PotentialPhishingEmailWarning w;
    QVERIFY(!w.isVisible());
    // Verify QVERIFY(w.isCloseButtonVisible());
    auto act = w.findChild<QAction *>(u"sendnow"_s);
    QVERIFY(act);
}

QTEST_MAIN(PotentialPhishingEmailWarningTest)

#include "moc_potentialphishingemailwarningtest.cpp"
