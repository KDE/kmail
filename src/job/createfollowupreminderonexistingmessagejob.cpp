/*
   SPDX-FileCopyrightText: 2014-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "createfollowupreminderonexistingmessagejob.h"
#include "kmail_debug.h"
#include <AkonadiCore/ItemFetchJob>
#include <AkonadiCore/ItemFetchScope>
#include <KMime/Message>
#include <MessageComposer/FollowupReminderCreateJob>

CreateFollowupReminderOnExistingMessageJob::CreateFollowupReminderOnExistingMessageJob(QObject *parent)
    : QObject(parent)
{
}

CreateFollowupReminderOnExistingMessageJob::~CreateFollowupReminderOnExistingMessageJob() = default;

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
    auto job = new Akonadi::ItemFetchJob(mMessageItem, this);
    job->fetchScope().fetchFullPayload(true);
    connect(job, &KJob::result, this, &CreateFollowupReminderOnExistingMessageJob::itemFetchJobDone);
}

void CreateFollowupReminderOnExistingMessageJob::itemFetchJobDone(KJob *job)
{
    auto fetchJob = qobject_cast<Akonadi::ItemFetchJob *>(job);
    if (fetchJob->items().count() == 1) {
        mMessageItem = fetchJob->items().constFirst();
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
    auto msg = mMessageItem.payload<KMime::Message::Ptr>();
    if (msg) {
        auto reminderJob = new MessageComposer::FollowupReminderCreateJob(this);
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

        connect(reminderJob, &KJob::result, this, &CreateFollowupReminderOnExistingMessageJob::slotReminderDone);
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
    } else {
        // TODO update dialog if opened
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

void CreateFollowupReminderOnExistingMessageJob::setDate(QDate date)
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
