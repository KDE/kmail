/*
   SPDX-FileCopyrightText: 2013-2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "folderarchivecache.h"
#include "folderarchiveaccountinfo.h"
#include "kmail_debug.h"

FolderArchiveCache::FolderArchiveCache(QObject *parent)
    : QObject(parent)
{
}

FolderArchiveCache::~FolderArchiveCache() = default;

void FolderArchiveCache::clearCache()
{
    mCache.clear();
}

void FolderArchiveCache::clearCacheWithContainsCollection(Akonadi::Collection::Id id)
{
    mCache.removeIf([id](auto it) {
        return it.value().colId == id;
    });
}

Akonadi::Collection::Id FolderArchiveCache::collectionId(FolderArchiveAccountInfo *info)
{
    auto it = mCache.find(info->instanceName());
    if (it == mCache.end()) {
        return -1;
    }
    // qCDebug(KMAIL_LOG)<<" Look at Cache ";
    // qCDebug(KMAIL_LOG)<<"instance name : "<<info->instanceName();
    switch (info->folderArchiveType()) {
    case FolderArchiveAccountInfo::FolderArchiveType::UniqueFolder:
        qCDebug(KMAIL_LOG) << "FolderArchiveAccountInfo::UniqueFolder has cache " << (*it).colId;
        return (*it).colId;
    case FolderArchiveAccountInfo::FolderArchiveType::FolderByMonths:
        // qCDebug(KMAIL_LOG)<<"FolderArchiveAccountInfo::ByMonths has cache ?";
        if ((*it).date.month() != QDate::currentDate().month()) {
            // qCDebug(KMAIL_LOG)<<"need to remove current cache month is not good";
            mCache.remove(info->instanceName());
            return -1;
        } else {
            return (*it).colId;
        }
    case FolderArchiveAccountInfo::FolderArchiveType::FolderByYears:
        // qCDebug(KMAIL_LOG)<<"FolderArchiveAccountInfo::ByYears has cache ?";
        if ((*it).date.year() != QDate::currentDate().year()) {
            // qCDebug(KMAIL_LOG)<<"need to remove current cache year is not good";
            mCache.remove(info->instanceName());
            return -1;
        } else {
            return (*it).colId;
        }
    }
    return (*it).colId;
}

void FolderArchiveCache::addToCache(const QString &resourceName, Akonadi::Collection::Id id)
{
    if (mCache.contains(resourceName)) {
        ArchiveCache cache = mCache.value(resourceName);
        cache.colId = id;
        mCache.insert(resourceName, cache);
    } else {
        ArchiveCache cache;
        cache.colId = id;
        mCache.insert(resourceName, cache);
    }
}

#include "moc_folderarchivecache.cpp"
