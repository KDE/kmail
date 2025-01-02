/*
   SPDX-FileCopyrightText: 2013-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <Akonadi/Collection>
#include <QDateTime>
#include <QHash>
#include <QObject>

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
    ~FolderArchiveCache() override;

    void addToCache(const QString &resourceName, Akonadi::Collection::Id id);

    [[nodiscard]] Akonadi::Collection::Id collectionId(FolderArchiveAccountInfo *info);

    void clearCacheWithContainsCollection(Akonadi::Collection::Id id);

    void clearCache();

private:
    QHash<QString, ArchiveCache> mCache;
};
