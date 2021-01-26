/*
   SPDX-FileCopyrightText: 2013-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "sendlaterremovemessagejob.h"
#include "sendlateragent_debug.h"
#include <ItemDeleteJob>

SendLaterRemoveMessageJob::SendLaterRemoveMessageJob(const QVector<Akonadi::Item::Id> &listItem, QObject *parent)
    : QObject(parent)
    , mListItems(listItem)
{
}

SendLaterRemoveMessageJob::~SendLaterRemoveMessageJob() = default;

void SendLaterRemoveMessageJob::start()
{
    deleteItem();
}

void SendLaterRemoveMessageJob::deleteItem()
{
    if (mIndex < mListItems.count()) {
        auto job = new Akonadi::ItemDeleteJob(Akonadi::Item(mListItems.at(mIndex)), this);
        connect(job, &Akonadi::ItemDeleteJob::result, this, &SendLaterRemoveMessageJob::slotItemDeleteDone);
    } else {
        deleteLater();
    }
}

void SendLaterRemoveMessageJob::slotItemDeleteDone(KJob *job)
{
    if (job->error()) {
        qCDebug(SENDLATERAGENT_LOG) << " Error during delete item :" << job->errorString();
    }
    ++mIndex;
    deleteItem();
}
