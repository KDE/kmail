/*
  SPDX-FileCopyrightText: 2015-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later

*/

#include "potentialphishingdetailwidgettest.h"
#include "../potentialphishingdetailwidget.h"
#include <QLabel>
#include <QListWidget>
#include <QTest>

PotentialPhishingDetailWidgetTest::PotentialPhishingDetailWidgetTest(QObject *parent)
    : QObject(parent)
{
}

PotentialPhishingDetailWidgetTest::~PotentialPhishingDetailWidgetTest() = default;

void PotentialPhishingDetailWidgetTest::shouldHaveDefaultValue()
{
    PotentialPhishingDetailWidget dlg;
    auto searchLabel = dlg.findChild<QLabel *>(QStringLiteral("label"));
    QVERIFY(searchLabel);

    auto listWidget = dlg.findChild<QListWidget *>(QStringLiteral("list_widget"));
    QVERIFY(listWidget);
    QCOMPARE(listWidget->count(), 0);
}

void PotentialPhishingDetailWidgetTest::shouldFillList()
{
    PotentialPhishingDetailWidget dlg;
    auto listWidget = dlg.findChild<QListWidget *>(QStringLiteral("list_widget"));
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
    auto listWidget = dlg.findChild<QListWidget *>(QStringLiteral("list_widget"));
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
    auto listWidget = dlg.findChild<QListWidget *>(QStringLiteral("list_widget"));
    QStringList lst;
    lst << QStringLiteral("bla");
    lst << QStringLiteral("blo");
    lst << QStringLiteral("blo");
    dlg.fillList(lst);
    QCOMPARE(listWidget->count(), (lst.count() - 1));
}

QTEST_MAIN(PotentialPhishingDetailWidgetTest)
