/*
   Copyright (C) 2014-2017 Montel Laurent <montel@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "followupreminderjob.h"

#include <AkonadiCore/ItemFetchJob>
#include <AkonadiCore/ItemFetchScope>
#include <Akonadi/KMime/MessageParts>

#include <KMime/Message>

#include "followupreminderagent_debug.h"

FollowUpReminderJob::FollowUpReminderJob(QObject *parent)
    : QObject(parent)
{
}

FollowUpReminderJob::~FollowUpReminderJob()
{

}

void FollowUpReminderJob::start()
{
    if (!mItem.isValid()) {
        qCDebug(FOLLOWUPREMINDERAGENT_LOG) << " item is not valid";
        deleteLater();
        return;
    }
    Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob(mItem);
    job->fetchScope().fetchPayloadPart(Akonadi::MessagePart::Envelope, true);
    job->fetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);

    connect(job, &Akonadi::ItemFetchJob::result, this, &FollowUpReminderJob::slotItemFetchJobDone);
}

void FollowUpReminderJob::setItem(const Akonadi::Item &item)
{
    mItem = item;
}

void FollowUpReminderJob::slotItemFetchJobDone(KJob *job)
{
    if (job->error()) {
        qCCritical(FOLLOWUPREMINDERAGENT_LOG) << "Error while fetching item. " << job->error() << job->errorString();
        deleteLater();
        return;
    }

    const Akonadi::ItemFetchJob *fetchJob = qobject_cast<Akonadi::ItemFetchJob *>(job);

    const Akonadi::Item::List items = fetchJob->items();
    if (items.isEmpty()) {
        qCCritical(FOLLOWUPREMINDERAGENT_LOG) << "Error while fetching item: item not found";
        deleteLater();
        return;
    }
    const Akonadi::Item item = items.at(0);
    if (!item.hasPayload<KMime::Message::Ptr>()) {
        qCCritical(FOLLOWUPREMINDERAGENT_LOG) << "Item has not payload";
        deleteLater();
        return;
    }
    const KMime::Message::Ptr msg = item.payload<KMime::Message::Ptr>();
    if (msg) {
        KMime::Headers::InReplyTo *replyTo = msg->inReplyTo(false);
        if (replyTo) {
            const QString replyToIdStr = replyTo->asUnicodeString();
            Q_EMIT finished(replyToIdStr, item.id());
        }
    }
    deleteLater();
}
