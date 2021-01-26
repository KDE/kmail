/*
  SPDX-FileCopyrightText: 2015-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#include "tagselectdialogtest.h"
#include "tag/tagselectdialog.h"
#include <KListWidgetSearchLine>
#include <QListWidget>
#include <QStandardPaths>
#include <QTest>

TagSelectDialogTest::TagSelectDialogTest(QObject *parent)
    : QObject(parent)
{
}

TagSelectDialogTest::~TagSelectDialogTest()
{
}

void TagSelectDialogTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
}

void TagSelectDialogTest::shouldHaveDefaultValue()
{
    TagSelectDialog dlg(nullptr, 1, Akonadi::Item());
    auto *listWidget = dlg.findChild<QListWidget *>(QStringLiteral("listtag"));
    QVERIFY(listWidget);

    auto *listWidgetSearchLine = dlg.findChild<KListWidgetSearchLine *>(QStringLiteral("searchline"));
    QVERIFY(listWidgetSearchLine);
    QVERIFY(listWidgetSearchLine->isClearButtonEnabled());
}

QTEST_MAIN(TagSelectDialogTest)
