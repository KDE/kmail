/*
   SPDX-FileCopyrightText: 2013-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "folderarchivemanager.h"
#include "folderarchiveaccountinfo.h"
#include "folderarchiveagentjob.h"
#include "folderarchivecache.h"
#include "folderarchiveutil.h"

#include "util.h"

#include <AkonadiCore/AgentManager>
#include <AkonadiCore/CollectionFetchJob>
#include <AkonadiCore/ItemFetchJob>
#include <AkonadiCore/ItemFetchScope>

#include "kmail_debug.h"
#include <KLocalizedString>
#include <KNotification>
#include <KSharedConfig>

#include <QRegularExpression>

FolderArchiveManager::FolderArchiveManager(QObject *parent)
    : QObject(parent)
    , mFolderArchiveCache(new FolderArchiveCache(this))
{
    load();
}

FolderArchiveManager::~FolderArchiveManager()
{
    qDeleteAll(mListAccountInfo);
    mListAccountInfo.clear();
    qDeleteAll(mJobQueue);
    delete mCurrentJob;
}

void FolderArchiveManager::slotCollectionRemoved(const Akonadi::Collection &collection)
{
    KConfig config(FolderArchive::FolderArchiveUtil::configFileName());
    mFolderArchiveCache->clearCacheWithContainsCollection(collection.id());
    for (FolderArchiveAccountInfo *info : std::as_const(mListAccountInfo)) {
        if (info->archiveTopLevel() == collection.id()) {
            info->setArchiveTopLevel(-1);
            KConfigGroup group = config.group(FolderArchive::FolderArchiveUtil::groupConfigPattern() + info->instanceName());
            info->writeConfig(group);
        }
    }
    load();
}

FolderArchiveAccountInfo *FolderArchiveManager::infoFromInstanceName(const QString &instanceName) const
{
    for (FolderArchiveAccountInfo *info : std::as_const(mListAccountInfo)) {
        if (info->instanceName() == instanceName) {
            return info;
        }
    }
    return nullptr;
}

void FolderArchiveManager::setArchiveItem(qlonglong itemId)
{
    auto job = new Akonadi::ItemFetchJob(Akonadi::Item(itemId), this);
    job->fetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);
    job->fetchScope().setFetchRemoteIdentification(true);
    connect(job, &Akonadi::ItemFetchJob::result, this, &FolderArchiveManager::slotFetchParentCollection);
}

void FolderArchiveManager::slotFetchParentCollection(KJob *job)
{
    if (job->error()) {
        moveFailed(i18n("Unable to fetch folder. Error reported: %1", job->errorString()));
        qCDebug(KMAIL_LOG) << "Unable to fetch folder:" << job->errorString();
        return;
    }
    const Akonadi::ItemFetchJob *fetchJob = qobject_cast<Akonadi::ItemFetchJob *>(job);
    const Akonadi::Item::List items = fetchJob->items();
    if (items.isEmpty()) {
        moveFailed(i18n("No folder returned."));
        qCDebug(KMAIL_LOG) << "Fetch list is empty";
    } else {
        auto jobCol = new Akonadi::CollectionFetchJob(Akonadi::Collection(items.first().parentCollection().id()), Akonadi::CollectionFetchJob::Base, this);
        jobCol->setProperty("itemId", items.first().id());
        connect(jobCol, &Akonadi::CollectionFetchJob::result, this, &FolderArchiveManager::slotFetchCollection);
    }
}

void FolderArchiveManager::slotFetchCollection(KJob *job)
{
    if (job->error()) {
        moveFailed(i18n("Unable to fetch parent folder. Error reported: %1", job->errorString()));
        qCDebug(KMAIL_LOG) << "cannot fetch collection " << job->errorString();
        return;
    }
    auto jobCol = qobject_cast<Akonadi::CollectionFetchJob *>(job);
    if (jobCol->collections().isEmpty()) {
        moveFailed(i18n("Unable to return list of folders."));
        qCDebug(KMAIL_LOG) << "List of folder is empty";
        return;
    }

    const Akonadi::Item::List itemIds = {Akonadi::Item(jobCol->property("itemId").toLongLong())};
    setArchiveItems(itemIds, jobCol->collections().constFirst().resource());
}

void FolderArchiveManager::setArchiveItems(const Akonadi::Item::List &items, const QString &instanceName)
{
    FolderArchiveAccountInfo *info = infoFromInstanceName(instanceName);
    if (info) {
        auto job = new FolderArchiveAgentJob(this, info, items);
        if (mCurrentJob) {
            mJobQueue.enqueue(job);
        } else {
            mCurrentJob = job;
            job->start();
        }
    }
}

void FolderArchiveManager::slotInstanceRemoved(const Akonadi::AgentInstance &instance)
{
    const QString instanceName = instance.name();
    for (FolderArchiveAccountInfo *info : std::as_const(mListAccountInfo)) {
        if (info->instanceName() == instanceName) {
            mListAccountInfo.removeAll(info);
            removeInfo(instanceName);
            break;
        }
    }
}

void FolderArchiveManager::removeInfo(const QString &instanceName)
{
    KConfig config(FolderArchive::FolderArchiveUtil::configFileName());
    KConfigGroup group = config.group(FolderArchive::FolderArchiveUtil::groupConfigPattern() + instanceName);
    group.deleteGroup();
    config.sync();
}

void FolderArchiveManager::load()
{
    qDeleteAll(mListAccountInfo);
    mListAccountInfo.clear();
    // Be sure to clear cache.
    mFolderArchiveCache->clearCache();

    KConfig config(FolderArchive::FolderArchiveUtil::configFileName());
    const QStringList accountList = config.groupList().filter(QRegularExpression(FolderArchive::FolderArchiveUtil::groupConfigPattern()));
    for (const QString &account : accountList) {
        KConfigGroup group = config.group(account);
        auto info = new FolderArchiveAccountInfo(group);
        if (info->enabled()) {
            mListAccountInfo.append(info);
        } else {
            delete info;
        }
    }
}

void FolderArchiveManager::moveDone()
{
    KNotification::event(QStringLiteral("folderarchivedone"),
                         QString(),
                         i18n("Messages archived"),
                         QStringLiteral("kmail"),
                         nullptr,
                         KNotification::CloseOnTimeout,
                         QStringLiteral("kmail2"));
    nextJob();
}

void FolderArchiveManager::moveFailed(const QString &msg)
{
    KNotification::event(QStringLiteral("folderarchiveerror"),
                         QString(),
                         msg,
                         QStringLiteral("kmail"),
                         nullptr,
                         KNotification::CloseOnTimeout,
                         QStringLiteral("kmail2"));
    nextJob();
}

void FolderArchiveManager::nextJob()
{
    mCurrentJob->deleteLater();
    if (mJobQueue.isEmpty()) {
        mCurrentJob = nullptr;
    } else {
        mCurrentJob = mJobQueue.dequeue();
        mCurrentJob->start();
    }
}

FolderArchiveCache *FolderArchiveManager::folderArchiveCache() const
{
    return mFolderArchiveCache;
}

void FolderArchiveManager::reloadConfig()
{
    load();
}
