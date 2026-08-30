/*
   SPDX-FileCopyrightText: 2014-2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "archivemailwidgettest.h"
#include "../archivemailwidget.h"

#include <QStandardPaths>
#include <QTest>
#include <QTreeWidget>
using namespace Qt::Literals::StringLiterals;

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
    ArchiveMailWidget mailwidget({}, &parent, {u"akonadi_archivemail_agent"_s});

    auto treeWidget = parent.findChild<QTreeWidget *>(u"treewidget"_s);
    QVERIFY(treeWidget);

    QCOMPARE(treeWidget->topLevelItemCount(), 0);
}

QTEST_MAIN(ArchiveMailWidgetTest)

#include "moc_archivemailwidgettest.cpp"
