/*
   SPDX-FileCopyrightText: 2013-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef FOLDERARCHIVEAGENTJOB_H
#define FOLDERARCHIVEAGENTJOB_H

#include <QObject>
#include <AkonadiCore/Item>
class KJob;
class FolderArchiveAccountInfo;
class FolderArchiveManager;
class KMMoveCommand;
class FolderArchiveAgentJob : public QObject
{
    Q_OBJECT
public:
    explicit FolderArchiveAgentJob(FolderArchiveManager *manager, FolderArchiveAccountInfo *info, const Akonadi::Item::List &lstItem, QObject *parent = nullptr);
    ~FolderArchiveAgentJob();

    void start();

private:
    Q_DISABLE_COPY(FolderArchiveAgentJob)
    void slotFetchCollection(KJob *job);
    void sloMoveMailsToCollection(const Akonadi::Collection &col);
    void slotCheckFailed(const QString &message);
    void slotCollectionIdFound(const Akonadi::Collection &col);
    void slotMoveMessages(KMMoveCommand *);

    void sendError(const QString &error);
    const Akonadi::Item::List mListItem;
    FolderArchiveManager *const mManager;
    FolderArchiveAccountInfo *const mInfo;
};

#endif // FOLDERARCHIVEAGENTJOB_H
