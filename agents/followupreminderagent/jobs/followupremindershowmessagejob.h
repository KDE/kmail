/*
   SPDX-FileCopyrightText: 2014-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef FOLLOWUPREMINDERSHOWMESSAGEJOB_H
#define FOLLOWUPREMINDERSHOWMESSAGEJOB_H

#include <AkonadiCore/Item>
#include <QObject>

class FollowUpReminderShowMessageJob : public QObject
{
    Q_OBJECT
public:
    explicit FollowUpReminderShowMessageJob(Akonadi::Item::Id id, QObject *parent = nullptr);
    ~FollowUpReminderShowMessageJob() override;

    void start();

private:
    Q_DISABLE_COPY(FollowUpReminderShowMessageJob)
    const Akonadi::Item::Id mId;
};

#endif // FOLLOWUPREMINDERSHOWMESSAGEJOB_H
