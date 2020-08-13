/*
   SPDX-FileCopyrightText: 2014-2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef FOLLOWUPREMINDERJOB_H
#define FOLLOWUPREMINDERJOB_H

#include <QObject>

#include <AkonadiCore/Item>

class FollowUpReminderJob : public QObject
{
    Q_OBJECT
public:
    explicit FollowUpReminderJob(QObject *parent = nullptr);
    ~FollowUpReminderJob();

    void setItem(const Akonadi::Item &item);

    void start();

Q_SIGNALS:
    void finished(const QString &messageId, Akonadi::Item::Id id);

private:
    Q_DISABLE_COPY(FollowUpReminderJob)
    void slotItemFetchJobDone(KJob *job);
    Akonadi::Item mItem;
};

#endif // FOLLOWUPREMINDERJOB_H
