/*
   Copyright (C) 2014-2020 Laurent Montel <montel@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "sendlaterconfiguredialogtest.h"
#include "../sendlaterconfiguredialog.h"
#include "../sendlaterconfigurewidget.h"

#include <QTreeWidget>
#include <QStandardPaths>
#include <QTest>

SendLaterConfigureDialogTest::SendLaterConfigureDialogTest(QObject *parent)
    : QObject(parent)
{
}

SendLaterConfigureDialogTest::~SendLaterConfigureDialogTest()
{
}

void SendLaterConfigureDialogTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
}

void SendLaterConfigureDialogTest::shouldHaveDefaultValue()
{
    SendLaterConfigureDialog dlg;
    SendLaterWidget *infowidget = dlg.findChild<SendLaterWidget *>(QStringLiteral("sendlaterwidget"));
    QVERIFY(infowidget);

    QTreeWidget *treeWidget = infowidget->findChild<QTreeWidget *>(QStringLiteral("treewidget"));
    QVERIFY(treeWidget);

    QCOMPARE(treeWidget->topLevelItemCount(), 0);
}

QTEST_MAIN(SendLaterConfigureDialogTest)
