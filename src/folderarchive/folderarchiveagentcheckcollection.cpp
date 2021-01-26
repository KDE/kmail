/*
   SPDX-FileCopyrightText: 2013-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "folderarchiveagentcheckcollection.h"
#include "folderarchiveaccountinfo.h"
#include "kmail_debug.h"

#include <KLocalizedString>

#include <AkonadiCore/CollectionCreateJob>
#include <AkonadiCore/CollectionFetchJob>

FolderArchiveAgentCheckCollection::FolderArchiveAgentCheckCollection(FolderArchiveAccountInfo *info, QObject *parent)
    : QObject(parent)
    , mCurrentDate(QDate::currentDate())
    , mInfo(info)
{
}

FolderArchiveAgentCheckCollection::~FolderArchiveAgentCheckCollection() = default;

void FolderArchiveAgentCheckCollection::start()
{
    Akonadi::Collection col(mInfo->archiveTopLevel());
    auto job = new Akonadi::CollectionFetchJob(col, Akonadi::CollectionFetchJob::FirstLevel);
    connect(job, &Akonadi::CollectionFetchJob::result, this, &FolderArchiveAgentCheckCollection::slotInitialCollectionFetchingFirstLevelDone);
}

void FolderArchiveAgentCheckCollection::slotInitialCollectionFetchingFirstLevelDone(KJob *job)
{
    if (job->error()) {
        qCWarning(KMAIL_LOG) << job->errorString();
        Q_EMIT checkFailed(i18n("Cannot fetch collection. %1", job->errorString()));
        return;
    }

    QString folderName;
    switch (mInfo->folderArchiveType()) {
    case FolderArchiveAccountInfo::UniqueFolder:
        // Nothing
        break;
    case FolderArchiveAccountInfo::FolderByMonths:
        // TODO translate ?
        folderName = QStringLiteral("%1-%2").arg(mCurrentDate.month()).arg(mCurrentDate.year());
        break;
    case FolderArchiveAccountInfo::FolderByYears:
        folderName = QStringLiteral("%1").arg(mCurrentDate.year());
        break;
    }

    if (folderName.isEmpty()) {
        Q_EMIT checkFailed(i18n("Folder name not defined."));
        return;
    }

    auto fetchJob = qobject_cast<Akonadi::CollectionFetchJob *>(job);

    const Akonadi::Collection::List cols = fetchJob->collections();
    for (const Akonadi::Collection &collection : cols) {
        if (collection.name() == folderName) {
            Q_EMIT collectionIdFound(collection);
            return;
        }
    }
    createNewFolder(folderName);
}

void FolderArchiveAgentCheckCollection::createNewFolder(const QString &name)
{
    Akonadi::Collection parentCollection(mInfo->archiveTopLevel());
    Akonadi::Collection collection;
    collection.setParentCollection(parentCollection);
    collection.setName(name);
    collection.setContentMimeTypes(QStringList() << QStringLiteral("message/rfc822"));

    auto job = new Akonadi::CollectionCreateJob(collection);
    connect(job, &Akonadi::CollectionCreateJob::result, this, &FolderArchiveAgentCheckCollection::slotCreateNewFolder);
}

void FolderArchiveAgentCheckCollection::slotCreateNewFolder(KJob *job)
{
    if (job->error()) {
        qCWarning(KMAIL_LOG) << job->errorString();
        Q_EMIT checkFailed(i18n("Unable to create folder. %1", job->errorString()));
        return;
    }
    auto createJob = qobject_cast<Akonadi::CollectionCreateJob *>(job);
    Q_EMIT collectionIdFound(createJob->collection());
}
