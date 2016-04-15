/*
  Copyright (c) 2013-2016 Montel Laurent <montel@kde.org>

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

#ifndef FOLDERARCHIVEMANAGER_H
#define FOLDERARCHIVEMANAGER_H

#include <QObject>
#include <QQueue>
#include <AkonadiCore/Item>
namespace Akonadi
{
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
    explicit FolderArchiveManager(QObject *parent = Q_NULLPTR);
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

private Q_SLOTS:
    void slotFetchParentCollection(KJob *job);
    void slotFetchCollection(KJob *job);

private:
    FolderArchiveAccountInfo *infoFromInstanceName(const QString &instanceName) const;
    void nextJob();
    void removeInfo(const QString &instanceName);
    QQueue<FolderArchiveAgentJob *> mJobQueue;
    FolderArchiveAgentJob *mCurrentJob;
    QList<FolderArchiveAccountInfo *> mListAccountInfo;
    FolderArchiveCache *mFolderArchiveCache;
};

#endif // FOLDERARCHIVEMANAGER_H
