/*
  Copyright (c) 2015-2016 Montel Laurent <montel@kde.org>

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

#include "potentialphishingdetailwidgettest.h"
#include "../potentialphishingdetailwidget.h"
#include <QLabel>
#include <QListWidget>
#include <qtest.h>

PotentialPhishingDetailWidgetTest::PotentialPhishingDetailWidgetTest(QObject *parent)
    : QObject(parent)
{

}

PotentialPhishingDetailWidgetTest::~PotentialPhishingDetailWidgetTest()
{

}

void PotentialPhishingDetailWidgetTest::shouldHaveDefaultValue()
{
    PotentialPhishingDetailWidget dlg;
    QLabel *searchLabel = dlg.findChild<QLabel *>(QStringLiteral("label"));
    QVERIFY(searchLabel);

    QListWidget *listWidget = dlg.findChild<QListWidget *>(QStringLiteral("list_widget"));
    QVERIFY(listWidget);
    QCOMPARE(listWidget->count(), 0);
}

void PotentialPhishingDetailWidgetTest::shouldFillList()
{
    PotentialPhishingDetailWidget dlg;
    QListWidget *listWidget = dlg.findChild<QListWidget *>(QStringLiteral("list_widget"));
    QStringList lst;
    lst << QStringLiteral("bla");
    lst << QStringLiteral("bli");
    lst << QStringLiteral("blo");
    dlg.fillList(lst);
    QCOMPARE(listWidget->count(), lst.count());
}

void PotentialPhishingDetailWidgetTest::shouldClearListBeforeToAddNew()
{
    PotentialPhishingDetailWidget dlg;
    QListWidget *listWidget = dlg.findChild<QListWidget *>(QStringLiteral("list_widget"));
    QStringList lst;
    lst << QStringLiteral("bla");
    lst << QStringLiteral("bli");
    lst << QStringLiteral("blo");
    dlg.fillList(lst);
    QCOMPARE(listWidget->count(), lst.count());
    lst.clear();
    lst << QStringLiteral("bla");
    lst << QStringLiteral("bli");
    dlg.fillList(lst);
    QCOMPARE(listWidget->count(), lst.count());
}

void PotentialPhishingDetailWidgetTest::shouldNotAddDuplicateEntries()
{
    PotentialPhishingDetailWidget dlg;
    QListWidget *listWidget = dlg.findChild<QListWidget *>(QStringLiteral("list_widget"));
    QStringList lst;
    lst << QStringLiteral("bla");
    lst << QStringLiteral("blo");
    lst << QStringLiteral("blo");
    dlg.fillList(lst);
    QCOMPARE(listWidget->count(), (lst.count() - 1));

}

QTEST_MAIN(PotentialPhishingDetailWidgetTest)
