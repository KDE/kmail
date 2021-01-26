/*
   SPDX-FileCopyrightText: 2013-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "folderarchiveagentjob.h"
#include "folderarchiveaccountinfo.h"
#include "folderarchiveagentcheckcollection.h"
#include "folderarchivecache.h"
#include "folderarchivemanager.h"

#include "kmcommands.h"

#include <AkonadiCore/CollectionFetchJob>
#include <AkonadiCore/ItemMoveJob>

#include <KLocalizedString>

FolderArchiveAgentJob::FolderArchiveAgentJob(FolderArchiveManager *manager, FolderArchiveAccountInfo *info, const Akonadi::Item::List &lstItem, QObject *parent)
    : QObject(parent)
    , mListItem(lstItem)
    , mManager(manager)
    , mInfo(info)
{
}

FolderArchiveAgentJob::~FolderArchiveAgentJob() = default;

void FolderArchiveAgentJob::start()
{
    if (!mInfo->isValid()) {
        sendError(i18n("Archive folder not defined. Please verify settings for account %1", mInfo->instanceName()));
        return;
    }
    if (mListItem.isEmpty()) {
        sendError(i18n("No messages selected."));
        return;
    }

    if (mInfo->folderArchiveType() == FolderArchiveAccountInfo::UniqueFolder) {
        auto fetchCollection = new Akonadi::CollectionFetchJob(Akonadi::Collection(mInfo->archiveTopLevel()), Akonadi::CollectionFetchJob::Base);
        connect(fetchCollection, &Akonadi::CollectionFetchJob::result, this, &FolderArchiveAgentJob::slotFetchCollection);
    } else {
        Akonadi::Collection::Id id = mManager->folderArchiveCache()->collectionId(mInfo);
        if (id != -1) {
            auto fetchCollection = new Akonadi::CollectionFetchJob(Akonadi::Collection(id), Akonadi::CollectionFetchJob::Base);
            connect(fetchCollection, &Akonadi::CollectionFetchJob::result, this, &FolderArchiveAgentJob::slotFetchCollection);
        } else {
            auto checkCol = new FolderArchiveAgentCheckCollection(mInfo, this);
            connect(checkCol, &FolderArchiveAgentCheckCollection::collectionIdFound, this, &FolderArchiveAgentJob::slotCollectionIdFound);
            connect(checkCol, &FolderArchiveAgentCheckCollection::checkFailed, this, &FolderArchiveAgentJob::slotCheckFailed);
            checkCol->start();
        }
    }
}

void FolderArchiveAgentJob::slotCheckFailed(const QString &message)
{
    sendError(i18n("Cannot fetch collection. %1", message));
}

void FolderArchiveAgentJob::slotFetchCollection(KJob *job)
{
    if (job->error()) {
        sendError(i18n("Cannot fetch collection. %1", job->errorString()));
        return;
    }
    auto fetchCollectionJob = static_cast<Akonadi::CollectionFetchJob *>(job);
    Akonadi::Collection::List collections = fetchCollectionJob->collections();
    if (collections.isEmpty()) {
        sendError(i18n("List of collections is empty. %1", job->errorString()));
        return;
    }
    sloMoveMailsToCollection(collections.at(0));
}

void FolderArchiveAgentJob::slotCollectionIdFound(const Akonadi::Collection &col)
{
    mManager->folderArchiveCache()->addToCache(mInfo->instanceName(), col.id());
    sloMoveMailsToCollection(col);
}

void FolderArchiveAgentJob::sloMoveMailsToCollection(const Akonadi::Collection &col)
{
    if (Akonadi::Collection::CanCreateItem & col.rights()) {
        auto command = new KMMoveCommand(col, mListItem, -1);
        connect(command, &KMMoveCommand::moveDone, this, &FolderArchiveAgentJob::slotMoveMessages);
        command->start();
    } else {
        sendError(i18n("This folder %1 is read only. Please verify the configuration of account %2", col.name(), mInfo->instanceName()));
    }
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
