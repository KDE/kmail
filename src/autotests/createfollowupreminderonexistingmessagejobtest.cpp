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
#include "createfollowupreminderonexistingmessagejobtest.h"
#include "../job/createfollowupreminderonexistingmessagejob.h"
#include <qtest.h>

CreateFollowupReminderOnExistingMessageJobTest::CreateFollowupReminderOnExistingMessageJobTest(QObject *parent)
    : QObject(parent)
{

}

CreateFollowupReminderOnExistingMessageJobTest::~CreateFollowupReminderOnExistingMessageJobTest()
{

}

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
