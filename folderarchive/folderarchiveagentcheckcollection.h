/*
  Copyright (c) 2013, 2014 Montel Laurent <montel@kde.org>

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

#ifndef FOLDERARCHIVEAGENTCHECKCOLLECTION_H
#define FOLDERARCHIVEAGENTCHECKCOLLECTION_H
#include <QObject>
#include <Akonadi/Collection>
#include <QDate>
class KJob;
class FolderArchiveAccountInfo;
class FolderArchiveAgentCheckCollection : public QObject
{
    Q_OBJECT
public:
    explicit FolderArchiveAgentCheckCollection(FolderArchiveAccountInfo *info, QObject *parent = 0);
    ~FolderArchiveAgentCheckCollection();

    void start();

Q_SIGNALS:
    void collectionIdFound(const Akonadi::Collection &col);
    void checkFailed(const QString &message);

private Q_SLOTS:
    void slotInitialCollectionFetchingDone(KJob*);
    void slotInitialCollectionFetchingFirstLevelDone(KJob *job);
    void slotCreateNewFolder(KJob*);

private:
    void createNewFolder(const QString &name);
    QDate mCurrentDate;
    FolderArchiveAccountInfo *mInfo;
};

#endif // FOLDERARCHIVEAGENTCHECKCOLLECTION_H
