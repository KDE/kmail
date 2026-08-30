/*
  SPDX-FileCopyrightText: 2015-2026 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "tagselectdialogtest.h"
#include "tag/tagselectdialog.h"
#include <KListWidgetSearchLine>
#include <QListWidget>
#include <QStandardPaths>
#include <QTest>
using namespace Qt::Literals::StringLiterals;

QTEST_MAIN(TagSelectDialogTest)

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
    auto listWidget = dlg.findChild<QListWidget *>(u"listtag"_s);
    QVERIFY(listWidget);

    auto listWidgetSearchLine = dlg.findChild<KListWidgetSearchLine *>(u"searchline"_s);
    QVERIFY(listWidgetSearchLine);
    QVERIFY(listWidgetSearchLine->isClearButtonEnabled());
}

#include "moc_tagselectdialogtest.cpp"
