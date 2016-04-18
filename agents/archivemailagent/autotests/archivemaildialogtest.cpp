/*
  Copyright (c) 2014-2016 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "archivemaildialogtest.h"
#include "../archivemaildialog.h"
#include <qtest.h>
#include <QTreeWidget>
#include "../archivemailwidget.h"

ArchiveMailDialogTest::ArchiveMailDialogTest(QObject *parent)
    : QObject(parent)
{

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
