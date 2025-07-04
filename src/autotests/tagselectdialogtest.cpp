/*
  SPDX-FileCopyrightText: 2015-2025 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
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

TagSelectDialogTest::~TagSelectDialogTest() = default;

void TagSelectDialogTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
}

void TagSelectDialogTest::shouldHaveDefaultValue()
{
    TagSelectDialog dlg(nullptr, 1, Akonadi::Item());
    auto listWidget = dlg.findChild<QListWidget *>(QStringLiteral("listtag"));
    QVERIFY(listWidget);

    auto listWidgetSearchLine = dlg.findChild<KListWidgetSearchLine *>(QStringLiteral("searchline"));
    QVERIFY(listWidgetSearchLine);
    QVERIFY(listWidgetSearchLine->isClearButtonEnabled());
}

QTEST_MAIN(TagSelectDialogTest)

#include "moc_tagselectdialogtest.cpp"
