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
    explicit FolderArchiveAgentJob(FolderArchiveManager *manager, FolderArchiveAccountInfo *info, const Akonadi::Item::List &lstItem, QObject *parent = Q_NULLPTR);
    ~FolderArchiveAgentJob();

    void start();

private Q_SLOTS:
    void slotFetchCollection(KJob *job);
    void sloMoveMailsToCollection(const Akonadi::Collection &col);
    void slotCheckFailder(const QString &message);
    void slotCollectionIdFound(const Akonadi::Collection &col);
    void slotMoveMessages(KMMoveCommand *);
private:
    void sendError(const QString &error);
    Akonadi::Item::List mListItem;
    FolderArchiveManager *mManager;
    FolderArchiveAccountInfo *mInfo;
};

#endif // FOLDERARCHIVEAGENTJOB_H
