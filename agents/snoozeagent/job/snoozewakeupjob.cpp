/*
   SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "snoozewakeupjob.h"
#include "snoozeagent_debug.h"
#include "snoozeinfo.h"
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
#if 0
    notification->setText(result.join(QLatin1Char('\n')));
    if (pixmap.isNull()) {
        notification->setIconName(mSpecialNotificationInfo.defaultIconName);
    } else {
        notification->setPixmap(pixmap);
    }

    auto showMailAction = notification->addAction(i18n("Show mail…"));
    connect(showMailAction, &KNotificationAction::activated, this, &SpecialNotifierJob::slotOpenMail);

    auto markAsReadAction = notification->addAction(i18n("Mark As Read"));
    connect(markAsReadAction, &KNotificationAction::activated, this, &SpecialNotifierJob::slotMarkAsRead);

    auto deleteAction = notification->addAction(i18n("Delete"));
    connect(deleteAction, &KNotificationAction::activated, this, &SpecialNotifierJob::slotDeleteMessage);

    if (NewMailNotifierAgentSettings::replyMail()) {
        QString replyLabel;
        switch (NewMailNotifierAgentSettings::replyMailType()) {
        case 0:
            replyLabel = i18n("Reply to Author");
            break;
        case 1:
            replyLabel = i18n("Reply to All");
            break;
        default:
            qCWarning(NEWMAILNOTIFIER_LOG) << " Problem with NewMailNotifierAgentSettings::replyMailType() value: "
                                           << NewMailNotifierAgentSettings::replyMailType();
            break;
        }

        if (!replyLabel.isEmpty()) {
            auto replyAction = notification->addAction(replyLabel);
            connect(replyAction, &KNotificationAction::activated, this, &SpecialNotifierJob::slotReplyMessage);
        }
    }
#endif
    connect(notification, &KNotification::closed, this, &SnoozeWakeUpJob::deleteLater);

    notification->sendEvent();

    // TODO KNotification "snoozemessagewokeup", with an "Open" action.
}

#include "moc_snoozewakeupjob.cpp"
