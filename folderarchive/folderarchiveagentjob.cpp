/*
  Copyright (c) 2013, 2014 Montel Laurent <montel@kde.org>

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

#include "folderarchiveagentjob.h"
#include "folderarchiveaccountinfo.h"
#include "folderarchiveagentcheckcollection.h"
#include "folderarchivemanager.h"
#include "folderarchivecache.h"

#include "kmcommands.h"

#include <AkonadiCore/ItemMoveJob>
#include <AkonadiCore/CollectionFetchJob>

#include <KLocalizedString>

FolderArchiveAgentJob::FolderArchiveAgentJob(FolderArchiveManager *manager, FolderArchiveAccountInfo *info, const QList<Akonadi::Item> &lstItem, QObject *parent)
    : QObject(parent),
      mLstItem(lstItem),
      mManager(manager),
      mInfo(info)
{
}

FolderArchiveAgentJob::~FolderArchiveAgentJob()
{
}

void FolderArchiveAgentJob::start()
{
    if (!mInfo->isValid()) {
        sendError(i18n("Archive folder not defined. Please verify settings for account %1", mInfo->instanceName()));
        return;
    }
    if (mLstItem.isEmpty()) {
        sendError(i18n("No messages selected."));
        return;
    }

    if (mInfo->folderArchiveType() == FolderArchiveAccountInfo::UniqueFolder) {
        Akonadi::CollectionFetchJob *fetchCollection = new Akonadi::CollectionFetchJob(Akonadi::Collection(mInfo->archiveTopLevel()), Akonadi::CollectionFetchJob::Base);
        connect(fetchCollection, &Akonadi::CollectionFetchJob::result, this, &FolderArchiveAgentJob::slotFetchCollection);
    } else {
        Akonadi::Collection::Id id = mManager->folderArchiveCache()->collectionId(mInfo);
        if (id != -1) {
            Akonadi::CollectionFetchJob *fetchCollection = new Akonadi::CollectionFetchJob(Akonadi::Collection(id), Akonadi::CollectionFetchJob::Base);
            connect(fetchCollection, &Akonadi::CollectionFetchJob::result, this, &FolderArchiveAgentJob::slotFetchCollection);
        } else {
            FolderArchiveAgentCheckCollection *checkCol = new FolderArchiveAgentCheckCollection(mInfo, this);
            connect(checkCol, &FolderArchiveAgentCheckCollection::collectionIdFound, this, &FolderArchiveAgentJob::slotCollectionIdFound);
            connect(checkCol, &FolderArchiveAgentCheckCollection::checkFailed, this, &FolderArchiveAgentJob::slotCheckFailder);
            checkCol->start();
        }
    }
}

void FolderArchiveAgentJob::slotCheckFailder(const QString &message)
{
    sendError(i18n("Cannot fetch collection. %1", message));
}

void FolderArchiveAgentJob::slotFetchCollection(KJob *job)
{
    if (job->error()) {
        sendError(i18n("Cannot fetch collection. %1", job->errorString()));
        return;
    }
    Akonadi::CollectionFetchJob *fetchCollectionJob = static_cast<Akonadi::CollectionFetchJob *>(job);
    Akonadi::Collection::List collections = fetchCollectionJob->collections();
    if (collections.isEmpty()) {
        sendError(i18n("List of collections is empty. %1", job->errorString()));
        return;
    }
    sloMoveMailsToCollection(collections.first());
}

void FolderArchiveAgentJob::slotCollectionIdFound(const Akonadi::Collection &col)
{
    mManager->folderArchiveCache()->addToCache(mInfo->instanceName(), col.id());
    sloMoveMailsToCollection(col);
}

void FolderArchiveAgentJob::sloMoveMailsToCollection(const Akonadi::Collection &col)
{
    KMMoveCommand *command = new KMMoveCommand(col, mLstItem, -1);
    connect(command, &KMMoveCommand::moveDone, this, &FolderArchiveAgentJob::slotMoveMessages);
    command->start();
}

void FolderArchiveAgentJob::sendError(const QString &error)
{
    mManager->moveFailed(error);
}

void FolderArchiveAgentJob::slotMoveMessages(KMMoveCommand *command)
{
    if (command->result() == KMCommand::Failed) {
        sendError(i18n("Cannot move messages."));
        return;
    }
    mManager->moveDone();
}
