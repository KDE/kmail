/*
   Copyright (C) 2014-2017 Montel Laurent <montel@kde.org>

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

#include "archivemaildialogtest.h"
#include "../archivemaildialog.h"
#include <qtest.h>
#include <QTreeWidget>
#include "../archivemailwidget.h"
#include <QStandardPaths>
ArchiveMailDialogTest::ArchiveMailDialogTest(QObject *parent)
    : QObject(parent)
{
    QStandardPaths::setTestModeEnabled(true);
}

ArchiveMailDialogTest::~ArchiveMailDialogTest()
{
}

void ArchiveMailDialogTest::shouldHaveDefaultValue()
{
    ArchiveMailDialog dlg;
    ArchiveMailWidget *mailwidget = dlg.findChild<ArchiveMailWidget *>(QStringLiteral("archivemailwidget"));
    QVERIFY(mailwidget);

    QTreeWidget *treeWidget = mailwidget->findChild<QTreeWidget *>(QStringLiteral("treewidget"));

    QVERIFY(treeWidget);

    QCOMPARE(treeWidget->topLevelItemCount(), 0);
}

QTEST_MAIN(ArchiveMailDialogTest)
