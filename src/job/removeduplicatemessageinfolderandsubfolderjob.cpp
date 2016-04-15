/*
  Copyright (c) 2015-2016 Montel Laurent <montel@kde.org>

  This library is free software; you can redistribute it and/or modify it
  under the terms of the GNU Library General Public License as published by
  the Free Software Foundation; either version 2 of the License, or (at your
  option) any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
  License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to the
  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.

*/

#include "removeduplicatemessageinfolderandsubfolderjob.h"
#include <PimCommon/FetchRecursiveCollectionsJob>
#include "kmail_debug.h"
#include <Akonadi/KMime/RemoveDuplicatesJob>

RemoveDuplicateMessageInFolderAndSubFolderJob::RemoveDuplicateMessageInFolderAndSubFolderJob(QObject *parent)
    : QObject(parent)
{

}

RemoveDuplicateMessageInFolderAndSubFolderJob::~RemoveDuplicateMessageInFolderAndSubFolderJob()
{

}

void RemoveDuplicateMessageInFolderAndSubFolderJob::start()
{
    if (mTopLevelCollection.isValid()) {
        PimCommon::FetchRecursiveCollectionsJob *fetchJob = new PimCommon::FetchRecursiveCollectionsJob(this);
        fetchJob->setTopCollection(mTopLevelCollection);
        connect(fetchJob, &PimCommon::FetchRecursiveCollectionsJob::fetchCollectionFailed, this, &RemoveDuplicateMessageInFolderAndSubFolderJob::slotFetchCollectionFailed);
        connect(fetchJob, &PimCommon::FetchRecursiveCollectionsJob::fetchCollectionFinished, this, &RemoveDuplicateMessageInFolderAndSubFolderJob::slotFetchCollectionDone);
        fetchJob->start();
    } else {
        qCDebug(KMAIL_LOG()) << "Invalid toplevel collection";
        deleteLater();
    }
}

void RemoveDuplicateMessageInFolderAndSubFolderJob::setTopLevelCollection(const Akonadi::Collection &topLevelCollection)
{
    mTopLevelCollection = topLevelCollection;
}

void RemoveDuplicateMessageInFolderAndSubFolderJob::slotFetchCollectionFailed()
{
    qCDebug(KMAIL_LOG()) << "Fetch toplevel collection failed";
    deleteLater();
}

void RemoveDuplicateMessageInFolderAndSubFolderJob::slotFetchCollectionDone(const Akonadi::Collection::List &list)
{
    Akonadi::RemoveDuplicatesJob *job = new Akonadi::RemoveDuplicatesJob(list, this);
    connect(job, &Akonadi::RemoveDuplicatesJob::finished, this, &RemoveDuplicateMessageInFolderAndSubFolderJob::slotFinished);
}

void RemoveDuplicateMessageInFolderAndSubFolderJob::slotFinished(KJob *job)
{
    if (job->error()) {
        qCDebug(KMAIL_LOG()) << " Error during remove duplicates " << job->errorString();
    }
    deleteLater();
}
