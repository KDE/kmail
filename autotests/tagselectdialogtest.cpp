/*
  Copyright (c) 2015 Montel Laurent <montel@kde.org>

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

#include "tagselectdialogtest.h"
#include "tag/tagselectdialog.h"
#include <KListWidgetSearchLine>
#include <QListWidget>
#include <qtest.h>

TagSelectDialogTest::TagSelectDialogTest(QObject *parent)
    : QObject(parent)
{

}

TagSelectDialogTest::~TagSelectDialogTest()
{

}

void TagSelectDialogTest::shouldHaveDefaultValue()
{
    TagSelectDialog dlg(0, 1, Akonadi::Item());
    QListWidget *listWidget = dlg.findChild<QListWidget *>(QLatin1String("listtag"));
    QVERIFY(listWidget);

    KListWidgetSearchLine *listWidgetSearchLine = dlg.findChild<KListWidgetSearchLine *>(QLatin1String("searchline"));
    QVERIFY(listWidgetSearchLine);
    //PORT KF5 QVERIFY(listWidgetSearchLine->isClearButtonShown());
    //PORT KF5 QVERIFY(listWidgetSearchLine->trapReturnKey());
}

QTEST_MAIN(TagSelectDialogTest)
