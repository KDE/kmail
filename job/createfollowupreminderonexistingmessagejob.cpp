/*
  Copyright (c) 2014 Montel Laurent <montel@kde.org>

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

#include <akonadi/itemfetchjob.h>
#include <Akonadi/ItemFetchScope>
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
        qDebug()<<" job can not started";
        deleteLater();
    }
}

void CreateFollowupReminderOnExistingMessageJob::doStart()
{
    Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob( mMessageItem , this );
    job->fetchScope().fetchFullPayload( true );
    connect( job, SIGNAL(result(KJob*)), SLOT(itemFetchJobDone(KJob*)) );
}

void CreateFollowupReminderOnExistingMessageJob::itemFetchJobDone(KJob* job)
{
    Akonadi::ItemFetchJob *fetchJob = qobject_cast<Akonadi::ItemFetchJob *>(job);
    if ( fetchJob->items().count() == 1 ) {
        mMessageItem = fetchJob->items().first();
    } else {
        qDebug()<<" CreateFollowupReminderOnExistingMessageJob Error during fetch: "<<job->errorString();
        deleteLater();
        return;
    }
    if ( !mMessageItem.hasPayload<KMime::Message::Ptr>() ) {
        qDebug()<<" item has not payload";
        deleteLater();
        return;
    }
    KMime::Message::Ptr msg =  mMessageItem.payload<KMime::Message::Ptr>();
    //TODO create followupreminderjob

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




