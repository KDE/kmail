/*
   SPDX-FileCopyrightText: 2014-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "createfollowupreminderonexistingmessagejobtest.h"
#include "../job/createfollowupreminderonexistingmessagejob.h"
#include <QTest>

CreateFollowupReminderOnExistingMessageJobTest::CreateFollowupReminderOnExistingMessageJobTest(QObject *parent)
    : QObject(parent)
{
}

CreateFollowupReminderOnExistingMessageJobTest::~CreateFollowupReminderOnExistingMessageJobTest() = default;

void CreateFollowupReminderOnExistingMessageJobTest::shouldHaveDefaultValue()
{
    CreateFollowupReminderOnExistingMessageJob job;
    QVERIFY(!job.date().isValid());
    QVERIFY(!job.messageItem().isValid());
    QVERIFY(!job.collection().isValid());
}

void CreateFollowupReminderOnExistingMessageJobTest::shouldValidBeforeStartJob()
{
    CreateFollowupReminderOnExistingMessageJob job;
    QVERIFY(!job.canStart());
    job.setDate(QDate::currentDate());
    QVERIFY(!job.canStart());
    Akonadi::Item item(42);
    job.setMessageItem(item);
    QVERIFY(!job.canStart());
    Akonadi::Collection col(42);
    job.setCollection(col);
    QVERIFY(job.canStart());
}

QTEST_MAIN(CreateFollowupReminderOnExistingMessageJobTest)
