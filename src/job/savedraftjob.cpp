/*
  SPDX-FileCopyrightText: 2014-2023 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later

*/

#include "savedraftjob.h"
#include "kmail_debug.h"
#include <Akonadi/Item>
#include <Akonadi/ItemCreateJob>
#include <Akonadi/MessageFlags>
#include <Akonadi/MessageStatus>

SaveDraftJob::SaveDraftJob(const KMime::Message::Ptr &msg, const Akonadi::Collection &col, QObject *parent)
    : KJob(parent)
    , mMsg(msg)
    , mCollection(col)
{
}

SaveDraftJob::~SaveDraftJob() = default;

void SaveDraftJob::start()
{
    Akonadi::Item item;
    item.setPayload(mMsg);
    item.setMimeType(KMime::Message::mimeType());
    item.setFlag(Akonadi::MessageFlags::Seen);
    Akonadi::MessageFlags::copyMessageFlags(*mMsg, item);

    auto createJob = new Akonadi::ItemCreateJob(item, mCollection);
    connect(createJob, &Akonadi::ItemCreateJob::result, this, &SaveDraftJob::slotStoreDone);
}

void SaveDraftJob::slotStoreDone(KJob *job)
{
    if (job->error()) {
        qCDebug(KMAIL_LOG) << " job->errorString() : " << job->errorString();
        setError(job->error());
        setErrorText(job->errorText());
    }
    emitResult();
}
