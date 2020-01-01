/*
   Copyright (C) 2013-2020 Laurent Montel <montel@kde.org>

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
#ifndef FOLDERARCHIVEMANAGER_H
#define FOLDERARCHIVEMANAGER_H

#include <QObject>
#include <QQueue>
#include <AkonadiCore/Item>
namespace Akonadi {
class AgentInstance;
class Collection;
}

class FolderArchiveAccountInfo;
class FolderArchiveAgentJob;
class FolderArchiveCache;
class KJob;
class FolderArchiveManager : public QObject
{
    Q_OBJECT
public:
    explicit FolderArchiveManager(QObject *parent = nullptr);
    ~FolderArchiveManager();

    void load();
    void setArchiveItems(const Akonadi::Item::List &items, const QString &instanceName);
    void setArchiveItem(qlonglong itemId);

    void moveFailed(const QString &msg);
    void moveDone();

    void collectionRemoved(const Akonadi::Collection &collection);

    FolderArchiveCache *folderArchiveCache() const;
    void reloadConfig();

public Q_SLOTS:
    void slotCollectionRemoved(const Akonadi::Collection &collection);
    void slotInstanceRemoved(const Akonadi::AgentInstance &instance);

private:
    Q_DISABLE_COPY(FolderArchiveManager)
    void slotFetchParentCollection(KJob *job);
    void slotFetchCollection(KJob *job);

    FolderArchiveAccountInfo *infoFromInstanceName(const QString &instanceName) const;
    void nextJob();
    void removeInfo(const QString &instanceName);
    QQueue<FolderArchiveAgentJob *> mJobQueue;
    FolderArchiveAgentJob *mCurrentJob = nullptr;
    QList<FolderArchiveAccountInfo *> mListAccountInfo;
    FolderArchiveCache *mFolderArchiveCache = nullptr;
};

#endif // FOLDERARCHIVEMANAGER_H
