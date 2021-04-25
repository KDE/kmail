/*
   SPDX-FileCopyrightText: 2013-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <AkonadiCore/Item>
#include <QObject>
#include <QQueue>
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
    explicit FolderArchiveManager(QObject *parent = nullptr);
    ~FolderArchiveManager() override;

    void load();
    void setArchiveItems(const Akonadi::Item::List &items, const QString &instanceName);
    void setArchiveItem(qlonglong itemId);

    void moveFailed(const QString &msg);
    void moveDone();

    Q_REQUIRED_RESULT FolderArchiveCache *folderArchiveCache() const;
    void reloadConfig();

public Q_SLOTS:
    void slotCollectionRemoved(const Akonadi::Collection &collection);
    void slotInstanceRemoved(const Akonadi::AgentInstance &instance);

private:
    Q_DISABLE_COPY(FolderArchiveManager)
    void slotFetchParentCollection(KJob *job);
    void slotFetchCollection(KJob *job);

    Q_REQUIRED_RESULT FolderArchiveAccountInfo *infoFromInstanceName(const QString &instanceName) const;
    void nextJob();
    void removeInfo(const QString &instanceName);
    QQueue<FolderArchiveAgentJob *> mJobQueue;
    FolderArchiveAgentJob *mCurrentJob = nullptr;
    QList<FolderArchiveAccountInfo *> mListAccountInfo;
    FolderArchiveCache *const mFolderArchiveCache;
};

