/*
   SPDX-FileCopyrightText: 2013-2021 Laurent Montel <montel@kde.org>

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
    QHash<QString, ArchiveCache>::iterator i = mCache.begin();
    while (i != mCache.end()) {
        if (i.value().colId == id) {
            i = mCache.erase(i);
        } else {
            ++i;
        }
    }
}

Akonadi::Collection::Id FolderArchiveCache::collectionId(FolderArchiveAccountInfo *info)
{
    // qCDebug(KMAIL_LOG)<<" Look at Cache ";
    if (mCache.contains(info->instanceName())) {
        // qCDebug(KMAIL_LOG)<<"instance name : "<<info->instanceName();
        switch (info->folderArchiveType()) {
        case FolderArchiveAccountInfo::UniqueFolder:
            qCDebug(KMAIL_LOG) << "FolderArchiveAccountInfo::UniqueFolder has cache " << mCache.value(info->instanceName()).colId;
            return mCache.value(info->instanceName()).colId;
        case FolderArchiveAccountInfo::FolderByMonths:
            // qCDebug(KMAIL_LOG)<<"FolderArchiveAccountInfo::ByMonths has cache ?";
            if (mCache.value(info->instanceName()).date.month() != QDate::currentDate().month()) {
                // qCDebug(KMAIL_LOG)<<"need to remove current cache month is not good";
                mCache.remove(info->instanceName());
                return -1;
            } else {
                return mCache.value(info->instanceName()).colId;
            }
        case FolderArchiveAccountInfo::FolderByYears:
            // qCDebug(KMAIL_LOG)<<"FolderArchiveAccountInfo::ByYears has cache ?";
            if (mCache.value(info->instanceName()).date.year() != QDate::currentDate().year()) {
                // qCDebug(KMAIL_LOG)<<"need to remove current cache year is not good";
                mCache.remove(info->instanceName());
                return -1;
            } else {
                return mCache.value(info->instanceName()).colId;
            }
        }
        return mCache.value(info->instanceName()).colId;
    }
    // qCDebug(KMAIL_LOG)<<" Don't have cache for this instancename "<<info->instanceName();
    return -1;
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
