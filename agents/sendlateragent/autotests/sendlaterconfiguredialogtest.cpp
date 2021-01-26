/*
   SPDX-FileCopyrightText: 2014-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "sendlaterconfiguredialogtest.h"
#include "../sendlaterconfiguredialog.h"
#include "../sendlaterconfigurewidget.h"

#include <QStandardPaths>
#include <QTest>
#include <QTreeWidget>

SendLaterConfigureDialogTest::SendLaterConfigureDialogTest(QObject *parent)
    : QObject(parent)
{
}

SendLaterConfigureDialogTest::~SendLaterConfigureDialogTest() = default;

void SendLaterConfigureDialogTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
}

void SendLaterConfigureDialogTest::shouldHaveDefaultValue()
{
    SendLaterConfigureDialog dlg;
    auto infowidget = dlg.findChild<SendLaterWidget *>(QStringLiteral("sendlaterwidget"));
    QVERIFY(infowidget);

    auto treeWidget = infowidget->findChild<QTreeWidget *>(QStringLiteral("treewidget"));
    QVERIFY(treeWidget);

    QCOMPARE(treeWidget->topLevelItemCount(), 0);
}

QTEST_MAIN(SendLaterConfigureDialogTest)
