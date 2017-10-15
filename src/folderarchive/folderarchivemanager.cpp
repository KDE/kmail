/*
   Copyright (C) 2013-2017 Montel Laurent <montel@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/
#include "folderarchivemanager.h"
#include "folderarchiveagentjob.h"
#include "folderarchiveaccountinfo.h"
#include "folderarchivecache.h"
#include "folderarchiveutil.h"

#include "util.h"

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

#include <QRegularExpression>

FolderArchiveManager::FolderArchiveManager(QObject *parent)
    : QObject(parent)
    , mCurrentJob(nullptr)
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
    for (FolderArchiveAccountInfo *info : qAsConst(mListAccountInfo)) {
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
    for (FolderArchiveAccountInfo *info : qAsConst(mListAccountInfo)) {
        if (info->instanceName() == instanceName) {
            return info;
        }
    }
    return nullptr;
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

    const Akonadi::Item::List itemIds = {Akonadi::Item(jobCol->property("itemId").toLongLong())};
    setArchiveItems(itemIds, jobCol->collections().constFirst().resource());
}

void FolderArchiveManager::setArchiveItems(const Akonadi::Item::List &items, const QString &instanceName)
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
    for (FolderArchiveAccountInfo *info : qAsConst(mListAccountInfo)) {
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
    const QStringList accountList = config.groupList().filter(QRegularExpression(FolderArchive::FolderArchiveUtil::groupConfigPattern()));
    for (const QString &account : accountList) {
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
                         nullptr,
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
