/*
   SPDX-FileCopyrightText: 2014-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef CREATEFOLLOWUPREMINDERONEXISTINGMESSAGEJOBTEST_H
#define CREATEFOLLOWUPREMINDERONEXISTINGMESSAGEJOBTEST_H

#include <QObject>

class CreateFollowupReminderOnExistingMessageJobTest : public QObject
{
    Q_OBJECT
public:
    explicit CreateFollowupReminderOnExistingMessageJobTest(QObject *parent = nullptr);
    ~CreateFollowupReminderOnExistingMessageJobTest();
private Q_SLOTS:
    void shouldHaveDefaultValue();
    void shouldValidBeforeStartJob();
};

#endif // CREATEFOLLOWUPREMINDERONEXISTINGMESSAGEJOBTEST_H
