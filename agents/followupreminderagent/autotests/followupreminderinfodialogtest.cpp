/*
   Copyright (C) 2014-2016 Montel Laurent <montel@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "followupreminderinfodialogtest.h"
#include "../followupreminderinfodialog.h"
#include "../followupreminderinfowidget.h"
#include "FollowupReminder/FollowUpReminderInfo"
#include <qtest.h>
#include <QStandardPaths>

FollowupReminderInfoDialogTest::FollowupReminderInfoDialogTest(QObject *parent)
    : QObject(parent)
{

}

FollowupReminderInfoDialogTest::~FollowupReminderInfoDialogTest()
{

}

void FollowupReminderInfoDialogTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
}

void FollowupReminderInfoDialogTest::shouldHaveDefaultValues()
{
    FollowUpReminderInfoDialog dlg;
    FollowUpReminderInfoWidget *infowidget = dlg.findChild<FollowUpReminderInfoWidget *>(QStringLiteral("FollowUpReminderInfoWidget"));
    QVERIFY(infowidget);

    QTreeWidget *treeWidget = infowidget->findChild<QTreeWidget *>(QStringLiteral("treewidget"));
    QVERIFY(treeWidget);

    QCOMPARE(treeWidget->topLevelItemCount(), 0);
}

void FollowupReminderInfoDialogTest::shouldAddItemInTreeList()
{
    FollowUpReminderInfoDialog dlg;
    FollowUpReminderInfoWidget *infowidget = dlg.findChild<FollowUpReminderInfoWidget *>(QStringLiteral("FollowUpReminderInfoWidget"));
    QTreeWidget *treeWidget = infowidget->findChild<QTreeWidget *>(QStringLiteral("treewidget"));
    QList<FollowUpReminder::FollowUpReminderInfo *> lstInfo;
    lstInfo.reserve(10);
    for (int i = 0; i < 10; ++i) {
        FollowUpReminder::FollowUpReminderInfo *info = new FollowUpReminder::FollowUpReminderInfo();
        lstInfo.append(info);
    }
    dlg.setInfo(lstInfo);
    //We load invalid infos.
    QCOMPARE(treeWidget->topLevelItemCount(), 0);

    //Load valid infos
    for (int i = 0; i < 10; ++i) {
        FollowUpReminder::FollowUpReminderInfo *info = new FollowUpReminder::FollowUpReminderInfo();
        info->setOriginalMessageItemId(42);
        info->setMessageId(QStringLiteral("foo"));
        info->setFollowUpReminderDate(QDate::currentDate());
        info->setTo(QStringLiteral("To"));
        lstInfo.append(info);
    }

    dlg.setInfo(lstInfo);
    QCOMPARE(treeWidget->topLevelItemCount(), 10);
}

void FollowupReminderInfoDialogTest::shouldItemHaveInfo()
{
    FollowUpReminderInfoDialog dlg;
    FollowUpReminderInfoWidget *infowidget = dlg.findChild<FollowUpReminderInfoWidget *>(QStringLiteral("FollowUpReminderInfoWidget"));
    QTreeWidget *treeWidget = infowidget->findChild<QTreeWidget *>(QStringLiteral("treewidget"));
    QList<FollowUpReminder::FollowUpReminderInfo *> lstInfo;

    //Load valid infos
    for (int i = 0; i < 10; ++i) {
        FollowUpReminder::FollowUpReminderInfo *info = new FollowUpReminder::FollowUpReminderInfo();
        info->setOriginalMessageItemId(42);
        info->setMessageId(QStringLiteral("foo"));
        info->setFollowUpReminderDate(QDate::currentDate());
        info->setTo(QStringLiteral("To"));
        lstInfo.append(info);
    }

    dlg.setInfo(lstInfo);
    for (int i = 0; i < treeWidget->topLevelItemCount(); ++i) {
        FollowUpReminderInfoItem *item = static_cast<FollowUpReminderInfoItem *>(treeWidget->topLevelItem(i));
        QVERIFY(item->info());
        QVERIFY(item->info()->isValid());
    }
}

QTEST_MAIN(FollowupReminderInfoDialogTest)
