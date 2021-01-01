/*
   SPDX-FileCopyrightText: 2014-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef CREATEFOLLOWUPREMINDERONEXISTINGMESSAGEJOB_H
#define CREATEFOLLOWUPREMINDERONEXISTINGMESSAGEJOB_H

#include "kmail_private_export.h"
#include <QObject>
#include <AkonadiCore/Collection>
#include <QDate>
#include <AkonadiCore/Item>

class KMAILTESTS_TESTS_EXPORT CreateFollowupReminderOnExistingMessageJob : public QObject
{
    Q_OBJECT
public:
    explicit CreateFollowupReminderOnExistingMessageJob(QObject *parent = nullptr);
    ~CreateFollowupReminderOnExistingMessageJob();

    void start();

    Q_REQUIRED_RESULT Akonadi::Collection collection() const;
    void setCollection(const Akonadi::Collection &collection);

    Q_REQUIRED_RESULT QDate date() const;
    void setDate(QDate date);

    Q_REQUIRED_RESULT Akonadi::Item messageItem() const;
    void setMessageItem(const Akonadi::Item &messageItem);

    Q_REQUIRED_RESULT bool canStart() const;

private:
    Q_DISABLE_COPY(CreateFollowupReminderOnExistingMessageJob)
    void itemFetchJobDone(KJob *job);
    void slotReminderDone(KJob *job);

    void doStart();
    Akonadi::Collection mCollection;
    Akonadi::Item mMessageItem;
    QDate mDate;
};

#endif // CREATEFOLLOWUPREMINDERONEXISTINGMESSAGEJOB_H
