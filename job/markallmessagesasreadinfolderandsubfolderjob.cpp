/*
  Copyright (c) 2015 Montel Laurent <montel@kde.org>

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

#include "markallmessagesasreadinfolderandsubfolderjob.h"
#include "fetchrecursivecollectionsjob.h"
#include "kmail_debug.h"

MarkAllMessagesAsReadInFolderAndSubFolderJob::MarkAllMessagesAsReadInFolderAndSubFolderJob(QObject *parent)
    : QObject(parent)
{

}

MarkAllMessagesAsReadInFolderAndSubFolderJob::~MarkAllMessagesAsReadInFolderAndSubFolderJob()
{

}

void MarkAllMessagesAsReadInFolderAndSubFolderJob::setTopLevelCollection(const Akonadi::Collection &topLevelCollection)
{
    mTopLevelCollection = topLevelCollection;
}

void MarkAllMessagesAsReadInFolderAndSubFolderJob::start()
{
    if (!mTopLevelCollection.isValid()) {
        qCDebug(KMAIL_LOG()) << "Invalid toplevel collection";
        deleteLater();
        return;
    }
    FetchRecursiveCollectionsJob *fetchJob = new FetchRecursiveCollectionsJob(this);
    fetchJob->setTopCollection(mTopLevelCollection);
    connect(fetchJob, &FetchRecursiveCollectionsJob::fetchCollectionFailed, this, &MarkAllMessagesAsReadInFolderAndSubFolderJob::slotFetchCollectionFailed);
    connect(fetchJob, &FetchRecursiveCollectionsJob::fetchCollectionFinished, this, &MarkAllMessagesAsReadInFolderAndSubFolderJob::slotFetchCollectionDone);
    fetchJob->start();
}

void MarkAllMessagesAsReadInFolderAndSubFolderJob::slotFetchCollectionFailed()
{
    qCDebug(KMAIL_LOG()) << "Fetch toplevel collection failed";
    deleteLater();
}

void MarkAllMessagesAsReadInFolderAndSubFolderJob::slotFetchCollectionDone(const Akonadi::Collection::List &list)
{
    Akonadi::MessageStatus messageStatus;
    messageStatus.setRead(true);
    Akonadi::MarkAsCommand *markAsReadAllJob = new Akonadi::MarkAsCommand(messageStatus, list);
    connect(markAsReadAllJob, &Akonadi::MarkAsCommand::result, this, &MarkAllMessagesAsReadInFolderAndSubFolderJob::slotMarkAsResult);
    markAsReadAllJob->execute();
}

void MarkAllMessagesAsReadInFolderAndSubFolderJob::slotMarkAsResult(Akonadi::MarkAsCommand::Result result)
{
    switch(result) {
    case Akonadi::MarkAsCommand::Undefined:
        qCDebug(KMAIL_LOG()) << "MarkAllMessagesAsReadInFolderAndSubFolderJob undefined result";
        break;
    case Akonadi::MarkAsCommand::OK:
        qCDebug(KMAIL_LOG()) << "MarkAllMessagesAsReadInFolderAndSubFoldeJob Done";
        break;
    case Akonadi::MarkAsCommand::Canceled:
        qCDebug(KMAIL_LOG()) << "MarkAllMessagesAsReadInFolderAndSubFoldeJob was canceled";
        break;
    case Akonadi::MarkAsCommand::Failed:
        qCDebug(KMAIL_LOG()) << "MarkAllMessagesAsReadInFolderAndSubFoldeJob was failed";
        break;
    }
    deleteLater();
}
