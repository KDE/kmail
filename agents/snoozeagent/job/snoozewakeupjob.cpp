/*
   SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "snoozewakeupjob.h"
#include "showmessagejob.h"
#include "snoozeagent_debug.h"
#include "snoozeinfo.h"
#include <KLocalizedString>
#include <KNotification>

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
}

void SnoozeWakeUpJob::notifyUser()
{
    auto notification = new KNotification(QStringLiteral("snoozemessagewokeup"), KNotification::CloseOnTimeout);

    auto showMailAction = notification->addAction(i18n("Show mail…"));
    connect(showMailAction, &KNotificationAction::activated, this, &SnoozeWakeUpJob::slotOpenMail);
    connect(notification, &KNotification::closed, this, &SnoozeWakeUpJob::deleteLater);

    notification->sendEvent();
}

void SnoozeWakeUpJob::slotOpenMail()
{
    auto job = new ShowMessageJob(mInfo->itemId());
    job->start();
    deleteLater();
}

#include "moc_snoozewakeupjob.cpp"
