/*
   SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "snoozeitemjob.h"
#include "snoozeagent_debug.h"
#include "snoozeutil.h"
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemModifyJob>
#include <Akonadi/LinkJob>
#include <MailCommon/SnoozeAttribute>

using namespace Snooze;
using namespace Qt::Literals::StringLiterals;

SnoozeItemJob::SnoozeItemJob(const Snooze::SnoozeInfo &info, QObject *parent)
    : QObject(parent)
    , mInfo(info)
{
}

SnoozeItemJob::~SnoozeItemJob() = default;

bool SnoozeItemJob::canStart() const
{
    return mInfo.isValid();
}

void SnoozeItemJob::start()
{
    if (!canStart()) {
        qCWarning(SNOOZEAGENT_LOG) << "Impossible to start SnoozeItemJob";
        failed(u"Invalid snooze info"_s);
        return;
    }
    auto job = new Akonadi::ItemFetchJob(Akonadi::Item(mInfo.itemId()), this);
    connect(job, &Akonadi::ItemFetchJob::result, this, &SnoozeItemJob::slotItemFetchDone);
}

void SnoozeItemJob::slotItemFetchDone(KJob *job)
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

    auto attribute = mItem.attribute<MailCommon::SnoozeAttribute>(Akonadi::Item::AddIfMissing);
    attribute->setWakeUpDateTime(mInfo.wakeUpDateTime());
    auto modifyJob = new Akonadi::ItemModifyJob(mItem, this);
    modifyJob->setIgnorePayload(true);
    connect(modifyJob, &Akonadi::ItemModifyJob::result, this, &SnoozeItemJob::slotItemModifyDone);
}

void SnoozeItemJob::slotItemModifyDone(KJob *job)
{
    if (job->error()) {
        failed(job->errorString());
        return;
    }
    const Akonadi::Collection::Id collectionId = SnoozeUtil::snoozeCollectionId();
    if (collectionId < 0) {
        finished();
        return;
    }
    auto linkJob = new Akonadi::LinkJob(Akonadi::Collection(collectionId), {mItem}, this);
    connect(linkJob, &Akonadi::LinkJob::result, this, &SnoozeItemJob::slotLinkDone);
}

void SnoozeItemJob::slotLinkDone(KJob *job)
{
    // Not fatal: the attribute alone already hides the message.
    if (job->error()) {
        qCWarning(SNOOZEAGENT_LOG) << "Impossible to link item" << mItem.id() << job->errorString();
    }
    finished();
}

void SnoozeItemJob::finished()
{
    Q_EMIT snoozeDone();
    deleteLater();
}

void SnoozeItemJob::failed(const QString &error)
{
    Q_EMIT snoozeFailed(error);
    deleteLater();
}

#include "moc_snoozeitemjob.cpp"
