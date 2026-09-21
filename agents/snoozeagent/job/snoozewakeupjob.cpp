/*
   SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "snoozewakeupjob.h"
#include "snoozeagent_debug.h"
#include "snoozeinfo.h"

using namespace Snooze;

SnoozeWakeUpJob::SnoozeWakeUpJob(Snooze::SnoozeInfo *info, QObject *parent)
    : QObject(parent)
    , mInfo(info)
{
}

SnoozeWakeUpJob::~SnoozeWakeUpJob() = default;

bool SnoozeWakeUpJob::canStart() const
{
    return mInfo && mInfo->isValid();
}

void SnoozeWakeUpJob::start()
{
    if (!canStart()) {
        qCWarning(SNOOZEAGENT_LOG) << "Impossible to start SnoozeWakeUpJob";
        deleteLater();
        return;
    }
    doStart();
}

void SnoozeWakeUpJob::doStart()
{
    // TODO Akonadi::ItemFetchJob on mInfo->itemId(), fetching SnoozeAttribute.
}

void SnoozeWakeUpJob::slotItemFetchDone(KJob *job)
{
    // TODO keep the item, then Akonadi::UnlinkJob from the virtual collection.
    Q_UNUSED(job)
}

void SnoozeWakeUpJob::slotUnlinkDone(KJob *job)
{
    // TODO remove SnoozeAttribute, honour SnoozeAgentSettings::markAsUnreadOnWakeUp(), Akonadi::ItemModifyJob.
    Q_UNUSED(job)
}

void SnoozeWakeUpJob::slotItemModifyDone(KJob *job)
{
    Q_UNUSED(job)
    notifyUser();
    Q_EMIT wakeUpDone();
    deleteLater();
}

void SnoozeWakeUpJob::notifyUser()
{
    // TODO KNotification "snoozemessagewokeup", with an "Open" action.
}

#include "moc_snoozewakeupjob.cpp"
