/*
  Copyright (c) 2013-2015 Montel Laurent <montel@kde.org>

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

#include "folderarchivemanager.h"
#include "folderarchiveagentjob.h"
#include "folderarchiveaccountinfo.h"
#include "folderarchivecache.h"
#include "folderarchiveutil.h"

#include <AkonadiCore/AgentManager>
#include <AkonadiCore/ItemFetchJob>
#include <AkonadiCore/ItemFetchScope>
#include <AkonadiCore/CollectionFetchJob>

#include <KSharedConfig>
#include <KNotification>
#include <KLocalizedString>
#include <QIcon>
#include <KIconLoader>
#include "kmail_debug.h"

FolderArchiveManager::FolderArchiveManager(QObject *parent)
    : QObject(parent),
      mCurrentJob(Q_NULLPTR)
{
    mFolderArchiveCache = new FolderArchiveCache(this);
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
    Q_FOREACH (FolderArchiveAccountInfo *info, mListAccountInfo) {
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
    Q_FOREACH (FolderArchiveAccountInfo *info, mListAccountInfo) {
        if (info->instanceName() == instanceName) {
            return info;
        }
    }
    return Q_NULLPTR;
}

void FolderArchiveManager::setArchiveItem(qlonglong itemId)
{
    Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob(Akonadi::Item(itemId), this);
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
        Akonadi::CollectionFetchJob *jobCol = new Akonadi::CollectionFetchJob(Akonadi::Collection(items.first().parentCollection().id()), Akonadi::CollectionFetchJob::Base, this);
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
    Akonadi::CollectionFetchJob *jobCol = qobject_cast<Akonadi::CollectionFetchJob *>(job);
    if (jobCol->collections().isEmpty()) {
        moveFailed(i18n("Unable to return list of folders."));
        qCDebug(KMAIL_LOG) << "List of folder is empty";
        return;
    }

    QList<Akonadi::Item> itemIds;
    itemIds << Akonadi::Item(jobCol->property("itemId").toLongLong());
    setArchiveItems(itemIds, jobCol->collections().first().resource());
}

void FolderArchiveManager::setArchiveItems(const QList<Akonadi::Item> &items, const QString &instanceName)
{
    FolderArchiveAccountInfo *info = infoFromInstanceName(instanceName);
    if (info) {
        FolderArchiveAgentJob *job = new FolderArchiveAgentJob(this, info, items);
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
    Q_FOREACH (FolderArchiveAccountInfo *info, mListAccountInfo) {
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
    //Be sure to clear cache.
    mFolderArchiveCache->clearCache();

    KConfig config(FolderArchive::FolderArchiveUtil::configFileName());
    const QStringList accountList = config.groupList().filter(QRegExp(FolderArchive::FolderArchiveUtil::groupConfigPattern()));
    Q_FOREACH (const QString &account, accountList) {
        KConfigGroup group = config.group(account);
        FolderArchiveAccountInfo *info = new FolderArchiveAccountInfo(group);
        if (info->enabled()) {
            mListAccountInfo.append(info);
        } else {
            delete info;
        }
    }
}

void FolderArchiveManager::moveDone()
{
    const QPixmap pixmap = QIcon::fromTheme(QStringLiteral("kmail")).pixmap(KIconLoader::SizeSmall, KIconLoader::SizeSmall);

    KNotification::event(QStringLiteral("folderarchivedone"),
                         i18n("Messages archived"),
                         pixmap,
                         Q_NULLPTR,
                         KNotification::CloseOnTimeout,
                         QStringLiteral("kmail2"));
    nextJob();
}

void FolderArchiveManager::moveFailed(const QString &msg)
{
    const QPixmap pixmap = QIcon::fromTheme(QStringLiteral("kmail")).pixmap(KIconLoader::SizeSmall, KIconLoader::SizeSmall);

    KNotification::event(QStringLiteral("folderarchiveerror"),
                         msg,
                         pixmap,
                         Q_NULLPTR,
                         KNotification::CloseOnTimeout,
                         QStringLiteral("kmail2"));
    nextJob();
}

void FolderArchiveManager::nextJob()
{
    mCurrentJob->deleteLater();
    if (mJobQueue.isEmpty()) {
        mCurrentJob = Q_NULLPTR;
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

