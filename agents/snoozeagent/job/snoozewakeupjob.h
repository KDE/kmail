/*
   SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <Akonadi/Item>
#include <QObject>

class KJob;

namespace Snooze
{
class SnoozeInfo;
}

/**
 * Wakes one snoozed message up: unlink it from the virtual Snooze collection,
 * drop its SnoozeAttribute, optionally mark it unread and notify.
 *
 * Nothing here is needed for the message to become visible again -- the
 * message list proxy already shows it once the deadline has passed. This job
 * only performs the clean-up and the user-visible notification.
 */
class SnoozeWakeUpJob : public QObject
{
    Q_OBJECT
public:
    explicit SnoozeWakeUpJob(Snooze::SnoozeInfo *info, QObject *parent = nullptr);
    ~SnoozeWakeUpJob() override;

    void start();

    [[nodiscard]] bool canStart() const;

Q_SIGNALS:
    void wakeUpDone();
    void wakeUpFailed(const QString &error);

private:
    void doStart();
    void slotItemFetchDone(KJob *job);
    void slotUnlinkDone(KJob *job);
    void slotItemModifyDone(KJob *job);
    void notifyUser();

    Snooze::SnoozeInfo *const mInfo;
    Akonadi::Item mItem;
};
