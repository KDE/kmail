/*
   SPDX-FileCopyrightText: 2014-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "archivemailwidgettest.h"
#include "../archivemailwidget.h"

#include <QStandardPaths>
#include <QTest>
#include <QTreeWidget>

ArchiveMailWidgetTest::ArchiveMailWidgetTest(QObject *parent)
    : QObject(parent)
{
    QStandardPaths::setTestModeEnabled(true);
}

ArchiveMailWidgetTest::~ArchiveMailWidgetTest() = default;

void ArchiveMailWidgetTest::shouldHaveDefaultValue()
{
    QWidget parent;
    new QHBoxLayout(&parent);
    ArchiveMailWidget mailwidget({}, &parent, {QStringLiteral("akonadi_archivemail_agent")});

    auto treeWidget = parent.findChild<QTreeWidget *>(QStringLiteral("treewidget"));
    QVERIFY(treeWidget);

    QCOMPARE(treeWidget->topLevelItemCount(), 0);
}

QTEST_MAIN(ArchiveMailWidgetTest)
