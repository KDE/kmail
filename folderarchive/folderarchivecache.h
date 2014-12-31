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

#ifndef FOLDERARCHIVECACHE_H
#define FOLDERARCHIVECACHE_H

#include <QObject>
#include <QDateTime>
#include <QHash>
#include <Akonadi/Collection>

class FolderArchiveAccountInfo;

struct ArchiveCache
{
    ArchiveCache()
        : date(QDate::currentDate()),
          colId(-1)
    {
    }

    QDate date;
    Akonadi::Collection::Id colId;
};

class FolderArchiveCache : public QObject
{
    Q_OBJECT
public:
    explicit FolderArchiveCache(QObject *parent = 0);
    ~FolderArchiveCache();

    void addToCache(const QString &resourceName, Akonadi::Collection::Id id);

    Akonadi::Collection::Id collectionId(FolderArchiveAccountInfo *info);

    void clearCacheWithContainsCollection(Akonadi::Collection::Id id);

    void clearCache();

private:
    QHash<QString, ArchiveCache> mCache;
};

#endif // FOLDERARCHIVECACHE_H
