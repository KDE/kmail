/*
  SPDX-FileCopyrightText: 2015-2026 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later

*/

#include "potentialphishingdetailwidgettest.h"
#include "../potentialphishingdetailwidget.h"
#include <QLabel>
#include <QListWidget>
#include <QStandardPaths>
#include <QTest>
using namespace Qt::Literals::StringLiterals;

PotentialPhishingDetailWidgetTest::PotentialPhishingDetailWidgetTest(QObject *parent)
    : QObject(parent)
{
    QStandardPaths::setTestModeEnabled(true);
}

PotentialPhishingDetailWidgetTest::~PotentialPhishingDetailWidgetTest() = default;

void PotentialPhishingDetailWidgetTest::shouldHaveDefaultValue()
{
    PotentialPhishingDetailWidget dlg;
    auto searchLabel = dlg.findChild<QLabel *>(u"label"_s);
    QVERIFY(searchLabel);

    auto listWidget = dlg.findChild<QListWidget *>(u"list_widget"_s);
    QVERIFY(listWidget);
    QCOMPARE(listWidget->count(), 0);
}

void PotentialPhishingDetailWidgetTest::shouldFillList()
{
    PotentialPhishingDetailWidget dlg;
    auto listWidget = dlg.findChild<QListWidget *>(u"list_widget"_s);
    QStringList lst;
    lst << u"bla"_s;
    lst << u"bli"_s;
    lst << u"blo"_s;
    dlg.fillList(lst);
    QCOMPARE(listWidget->count(), lst.count());
}

void PotentialPhishingDetailWidgetTest::shouldClearListBeforeToAddNew()
{
    PotentialPhishingDetailWidget dlg;
    auto listWidget = dlg.findChild<QListWidget *>(u"list_widget"_s);
    QStringList lst;
    lst << u"bla"_s;
    lst << u"bli"_s;
    lst << u"blo"_s;
    dlg.fillList(lst);
    QCOMPARE(listWidget->count(), lst.count());
    lst.clear();
    lst << u"bla"_s;
    lst << u"bli"_s;
    dlg.fillList(lst);
    QCOMPARE(listWidget->count(), lst.count());
}

void PotentialPhishingDetailWidgetTest::shouldNotAddDuplicateEntries()
{
    PotentialPhishingDetailWidget dlg;
    auto listWidget = dlg.findChild<QListWidget *>(u"list_widget"_s);
    QStringList lst;
    lst << u"bla"_s;
    lst << u"blo"_s;
    lst << u"blo"_s;
    dlg.fillList(lst);
    QCOMPARE(listWidget->count(), (lst.count() - 1));
}

QTEST_MAIN(PotentialPhishingDetailWidgetTest)

#include "moc_potentialphishingdetailwidgettest.cpp"
