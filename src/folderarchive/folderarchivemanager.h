/*
   SPDX-FileCopyrightText: 2013-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
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
    ~FolderArchiveManager() override;

    void load();
    void setArchiveItems(const Akonadi::Item::List &items, const QString &instanceName);
    void setArchiveItem(qlonglong itemId);

    void moveFailed(const QString &msg);
    void moveDone();

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
    FolderArchiveCache *const mFolderArchiveCache;
};

#endif // FOLDERARCHIVEMANAGER_H
