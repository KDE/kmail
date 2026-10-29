/*
   SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "kmail_private_export.h"
#include <Akonadi/Item>
#include <QDateTime>
#include <QObject>
class KJob;

class KMAILTESTS_TESTS_EXPORT SnoozeMessagesJob : public QObject
{
    Q_OBJECT
public:
    explicit SnoozeMessagesJob(QObject *parent = nullptr);
    ~SnoozeMessagesJob() override;

    void start();

    [[nodiscard]] Akonadi::Item::List items() const;
    void setItems(const Akonadi::Item::List &items);

    [[nodiscard]] QDateTime wakeUpDateTime() const;
    void setWakeUpDateTime(const QDateTime &wakeUpDateTime);

    [[nodiscard]] bool canStart() const;

private:
    KMAIL_NO_EXPORT void slotItemFetchDone(KJob *job);

    Akonadi::Item::List mItems;
    QDateTime mWakeUpDateTime;
};
