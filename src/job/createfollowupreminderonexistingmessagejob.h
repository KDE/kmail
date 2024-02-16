/*
   SPDX-FileCopyrightText: 2014-2024 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "kmail_private_export.h"
#include <Akonadi/Collection>
#include <Akonadi/Item>
#include <QDate>
#include <QObject>

class KMAILTESTS_TESTS_EXPORT CreateFollowupReminderOnExistingMessageJob : public QObject
{
    Q_OBJECT
public:
    explicit CreateFollowupReminderOnExistingMessageJob(QObject *parent = nullptr);
    ~CreateFollowupReminderOnExistingMessageJob() override;

    void start();

    [[nodiscard]] Akonadi::Collection collection() const;
    void setCollection(const Akonadi::Collection &collection);

    [[nodiscard]] QDate date() const;
    void setDate(QDate date);

    [[nodiscard]] Akonadi::Item messageItem() const;
    void setMessageItem(const Akonadi::Item &messageItem);

    [[nodiscard]] bool canStart() const;

private:
    KMAIL_NO_EXPORT void itemFetchJobDone(KJob *job);
    KMAIL_NO_EXPORT void slotReminderDone(KJob *job);

    KMAIL_NO_EXPORT void doStart();
    Akonadi::Collection mCollection;
    Akonadi::Item mMessageItem;
    QDate mDate;
};
