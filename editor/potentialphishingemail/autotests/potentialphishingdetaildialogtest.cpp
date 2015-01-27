/*
  Copyright (c) 2015 Montel Laurent <montel@kde.org>

  This library is free software; you can redistribute it and/or modify it
  under the terms of the GNU Library General Public License as published by
  the Free Software Foundation; either version 2 of the License, or (at your
  option) any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
  License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to the
  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.

*/

#include "potentialphishingdetaildialogtest.h"
#include "../potentialphishingdetaildialog.h"
#include <QLabel>
#include <QListWidget>
#include <qtest_kde.h>

PotentialPhishingDetailDialogTest::PotentialPhishingDetailDialogTest(QObject *parent)
    : QObject(parent)
{

}

PotentialPhishingDetailDialogTest::~PotentialPhishingDetailDialogTest()
{

}

void PotentialPhishingDetailDialogTest::shouldHaveDefaultValue()
{
    PotentialPhishingDetailDialog dlg;
    QLabel *searchLabel = qFindChild<QLabel *>(&dlg, QLatin1String("label"));
    QVERIFY(searchLabel);

    QListWidget *listWidget = qFindChild<QListWidget *>(&dlg, QLatin1String("list_widget"));
    QVERIFY(listWidget);
    QCOMPARE(listWidget->count(), 0);
}

void PotentialPhishingDetailDialogTest::shouldFillList()
{
    PotentialPhishingDetailDialog dlg;
    QListWidget *listWidget = qFindChild<QListWidget *>(&dlg, QLatin1String("list_widget"));
    QStringList lst;
    lst << QLatin1String("bla");
    lst << QLatin1String("bli");
    lst << QLatin1String("blo");
    dlg.fillList(lst);
    QCOMPARE(listWidget->count(), lst.count());
}

void PotentialPhishingDetailDialogTest::shouldClearListBeforeToAddNew()
{
    PotentialPhishingDetailDialog dlg;
    QListWidget *listWidget = qFindChild<QListWidget *>(&dlg, QLatin1String("list_widget"));
    QStringList lst;
    lst << QLatin1String("bla");
    lst << QLatin1String("bli");
    lst << QLatin1String("blo");
    dlg.fillList(lst);
    QCOMPARE(listWidget->count(), lst.count());
    lst.clear();
    lst << QLatin1String("bla");
    lst << QLatin1String("bli");
    dlg.fillList(lst);
    QCOMPARE(listWidget->count(), lst.count());
}

void PotentialPhishingDetailDialogTest::shouldNotAddDuplicateEntries()
{
    PotentialPhishingDetailDialog dlg;
    QListWidget *listWidget = qFindChild<QListWidget *>(&dlg, QLatin1String("list_widget"));
    QStringList lst;
    lst << QLatin1String("bla");
    lst << QLatin1String("blo");
    lst << QLatin1String("blo");
    dlg.fillList(lst);
    QCOMPARE(listWidget->count(), (lst.count()-1));

}

QTEST_KDEMAIN(PotentialPhishingDetailDialogTest, GUI)
