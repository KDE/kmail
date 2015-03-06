/*
  Copyright (c) 2014-2015 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "createfollowupreminderonexistingmessagejob.h"
#include "kmail_debug.h"
#include "../followupreminder/followupremindercreatejob.h"
#include <AkonadiCore/ItemFetchJob>
#include <AkonadiCore/ItemFetchScope>
#include <KMime/Message>

CreateFollowupReminderOnExistingMessageJob::CreateFollowupReminderOnExistingMessageJob(QObject *parent)
    : QObject(parent)
{

}

CreateFollowupReminderOnExistingMessageJob::~CreateFollowupReminderOnExistingMessageJob()
{

}

void CreateFollowupReminderOnExistingMessageJob::start()
{
    if (canStart()) {
        doStart();
    } else {
        qCDebug(KMAIL_LOG) << " job can not started";
        deleteLater();
    }
}

void CreateFollowupReminderOnExistingMessageJob::doStart()
{
    Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob(mMessageItem , this);
    job->fetchScope().fetchFullPayload(true);
    connect(job, SIGNAL(result(KJob*)), SLOT(itemFetchJobDone(KJob*)));
}

void CreateFollowupReminderOnExistingMessageJob::itemFetchJobDone(KJob *job)
{
    Akonadi::ItemFetchJob *fetchJob = qobject_cast<Akonadi::ItemFetchJob *>(job);
    if (fetchJob->items().count() == 1) {
        mMessageItem = fetchJob->items().first();
    } else {
        qCDebug(KMAIL_LOG) << " CreateFollowupReminderOnExistingMessageJob Error during fetch: " << job->errorString();
        deleteLater();
        return;
    }
    if (!mMessageItem.hasPayload<KMime::Message::Ptr>()) {
        qCDebug(KMAIL_LOG) << " item has not payload";
        deleteLater();
        return;
    }
    KMime::Message::Ptr msg =  mMessageItem.payload<KMime::Message::Ptr>();
    if (msg) {
        MessageComposer::FollowupReminderCreateJob *reminderJob = new MessageComposer::FollowupReminderCreateJob(this);
        KMime::Headers::MessageID *messageID = msg->messageID(false);
        if (messageID) {
            const QString messageIdStr = messageID->asUnicodeString();
            reminderJob->setMessageId(messageIdStr);
        } else {
            qCDebug(KMAIL_LOG) << " missing messageId";
            delete reminderJob;
            deleteLater();
            return;
        }

        reminderJob->setFollowUpReminderDate(mDate);
        reminderJob->setCollectionToDo(mCollection);
        reminderJob->setOriginalMessageItemId(mMessageItem.id());
        KMime::Headers::To *to = msg->to(false);
        if (to) {
            reminderJob->setTo(to->asUnicodeString());
        }
        KMime::Headers::Subject *subject = msg->subject(false);
        if (subject) {
            reminderJob->setSubject(subject->asUnicodeString());
        }

        connect(reminderJob, SIGNAL(result(KJob*)), this, SLOT(slotReminderDone(KJob*)));
        reminderJob->start();
    } else {
        qCDebug(KMAIL_LOG) << " no message found";
        deleteLater();
    }
}

void CreateFollowupReminderOnExistingMessageJob::slotReminderDone(KJob *job)
{
    if (job->error()) {
        qCDebug(KMAIL_LOG) << "CreateFollowupReminderOnExistingMessageJob::slotReminderDone  :" << job->errorString();
    }
    deleteLater();
}

Akonadi::Collection CreateFollowupReminderOnExistingMessageJob::collection() const
{
    return mCollection;
}

void CreateFollowupReminderOnExistingMessageJob::setCollection(const Akonadi::Collection &collection)
{
    mCollection = collection;
}

QDate CreateFollowupReminderOnExistingMessageJob::date() const
{
    return mDate;
}

void CreateFollowupReminderOnExistingMessageJob::setDate(const QDate &date)
{
    mDate = date;
}
Akonadi::Item CreateFollowupReminderOnExistingMessageJob::messageItem() const
{
    return mMessageItem;
}

void CreateFollowupReminderOnExistingMessageJob::setMessageItem(const Akonadi::Item &messageItem)
{
    mMessageItem = messageItem;
}

bool CreateFollowupReminderOnExistingMessageJob::canStart() const
{
    return mMessageItem.isValid() && mCollection.isValid() && mDate.isValid();
}

