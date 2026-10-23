/*
   SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "snoozewakeupjob.h"
#include "showmessagejob.h"
#include "snoozeagent_debug.h"
#include "snoozeagentsettings.h"
#include "snoozeutil.h"
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/ItemModifyJob>
#include <Akonadi/MessageFlags>
#include <Akonadi/UnlinkJob>
#include <KLocalizedString>
#include <KNotification>
#include <MailCommon/SnoozeAttribute>

using namespace Snooze;
using namespace Qt::Literals::StringLiterals;

SnoozeWakeUpJob::SnoozeWakeUpJob(const Snooze::SnoozeInfo &info, QObject *parent)
    : QObject(parent)
    , mInfo(info)
{
}

SnoozeWakeUpJob::~SnoozeWakeUpJob() = default;

bool SnoozeWakeUpJob::canStart() const
{
    return mInfo.isValid();
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
    auto job = new Akonadi::ItemFetchJob(Akonadi::Item(mInfo.itemId()), this);
    job->fetchScope().fetchAttribute<MailCommon::SnoozeAttribute>();
    connect(job, &Akonadi::ItemFetchJob::finished, this, &SnoozeWakeUpJob::slotItemFetchDone);
    job->start();
}

void SnoozeWakeUpJob::slotItemFetchDone(KJob *job)
{
    if (job->error()) {
        failed(job->errorString());
        return;
    }
    const Akonadi::Item::List items = static_cast<Akonadi::ItemFetchJob *>(job)->items();
    if (items.isEmpty()) {
        failed(u"Item %1 not found"_s.arg(mInfo.itemId()));
        return;
    }
    mItem = items.first();

    const Akonadi::Collection::Id collectionId = SnoozeUtil::snoozeCollectionId();
    if (collectionId < 0) {
        modifyItem();
        return;
    }
    auto unlinkJob = new Akonadi::UnlinkJob(Akonadi::Collection(collectionId), {mItem}, this);
    connect(unlinkJob, &Akonadi::UnlinkJob::result, this, &SnoozeWakeUpJob::slotUnlinkDone);
}

void SnoozeWakeUpJob::slotUnlinkDone(KJob *job)
{
    if (job->error()) {
        qCWarning(SNOOZEAGENT_LOG) << "Impossible to unlink item" << mItem.id() << job->errorString();
    }
    modifyItem();
}

void SnoozeWakeUpJob::modifyItem()
{
    mItem.removeAttribute<MailCommon::SnoozeAttribute>();
    if (SnoozeAgentSettings::self()->markAsUnreadOnWakeUp()) {
        mItem.clearFlag(Akonadi::MessageFlags::Seen);
    }
    auto modifyJob = new Akonadi::ItemModifyJob(mItem, this);
    modifyJob->setIgnorePayload(true);
    connect(modifyJob, &Akonadi::ItemModifyJob::result, this, &SnoozeWakeUpJob::slotItemModifyDone);
}

void SnoozeWakeUpJob::slotItemModifyDone(KJob *job)
{
    if (job->error()) {
        failed(job->errorString());
        return;
    }
    notifyUser();
    Q_EMIT wakeUpDone();
}

void SnoozeWakeUpJob::failed(const QString &error)
{
    Q_EMIT wakeUpFailed(error);
    deleteLater();
}

void SnoozeWakeUpJob::notifyUser()
{
    auto notification = new KNotification(u"snoozemessagewokeup"_s, KNotification::CloseOnTimeout);
    notification->setComponentName(u"akonadi_snooze_agent"_s);
    notification->setIconName(u"kmail"_s);
    notification->setTitle(i18n("Snoozed message is back"));
    notification->setText(mInfo.from().isEmpty() ? mInfo.subject() : i18nc("%1: subject, %2: sender", "%1\nFrom: %2", mInfo.subject(), mInfo.from()));

    auto showMailAction = notification->addAction(i18n("Show mail…"));
    connect(showMailAction, &KNotificationAction::activated, this, &SnoozeWakeUpJob::slotOpenMail);
    connect(notification, &KNotification::closed, this, &SnoozeWakeUpJob::deleteLater);

    notification->sendEvent();
}

void SnoozeWakeUpJob::slotOpenMail()
{
    auto job = new ShowMessageJob(mInfo.itemId());
    job->start();
    deleteLater();
}

#include "moc_snoozewakeupjob.cpp"
