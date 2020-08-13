/*
   SPDX-FileCopyrightText: 2013-2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "sendlaterremovemessagejob.h"
#include <ItemDeleteJob>
#include "sendlateragent_debug.h"

SendLaterRemoveMessageJob::SendLaterRemoveMessageJob(const QVector<Akonadi::Item::Id> &listItem, QObject *parent)
    : QObject(parent)
    , mListItems(listItem)
    , mIndex(0)
{
}

SendLaterRemoveMessageJob::~SendLaterRemoveMessageJob()
{
}

void SendLaterRemoveMessageJob::start()
{
    deleteItem();
}

void SendLaterRemoveMessageJob::deleteItem()
{
    if (mIndex < mListItems.count()) {
        Akonadi::ItemDeleteJob *job = new Akonadi::ItemDeleteJob(Akonadi::Item(mListItems.at(mIndex)), this);
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
