/*
   Copyright (C) 2013-2019 Montel Laurent <montel@kde.org>

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
#ifndef FOLDERARCHIVECACHE_H
#define FOLDERARCHIVECACHE_H

#include <QObject>
#include <QDateTime>
#include <QHash>
#include <AkonadiCore/Collection>

class FolderArchiveAccountInfo;

struct ArchiveCache {
    QDate date = QDate::currentDate();
    Akonadi::Collection::Id colId = -1;
};

class FolderArchiveCache : public QObject
{
    Q_OBJECT
public:
    explicit FolderArchiveCache(QObject *parent = nullptr);
    ~FolderArchiveCache();

    void addToCache(const QString &resourceName, Akonadi::Collection::Id id);

    Q_REQUIRED_RESULT Akonadi::Collection::Id collectionId(FolderArchiveAccountInfo *info);

    void clearCacheWithContainsCollection(Akonadi::Collection::Id id);

    void clearCache();

private:
    Q_DISABLE_COPY(FolderArchiveCache)
    QHash<QString, ArchiveCache> mCache;
};

#endif // FOLDERARCHIVECACHE_H
