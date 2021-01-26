/*
  SPDX-FileCopyrightText: 2015-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later

*/

#include "potentialphishingdetaildialogtest.h"
#include "../potentialphishingdetaildialog.h"
#include "../potentialphishingdetailwidget.h"
#include <QStandardPaths>
#include <QTest>
PotentialPhishingDetailDialogTest::PotentialPhishingDetailDialogTest(QObject *parent)
    : QObject(parent)
{
}

PotentialPhishingDetailDialogTest::~PotentialPhishingDetailDialogTest() = default;

void PotentialPhishingDetailDialogTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
}

void PotentialPhishingDetailDialogTest::shouldHaveDefaultValue()
{
    PotentialPhishingDetailDialog dlg;
    auto w = dlg.findChild<PotentialPhishingDetailWidget *>(QStringLiteral("potentialphising_widget"));
    QVERIFY(w);
}

QTEST_MAIN(PotentialPhishingDetailDialogTest)
