/*
   SPDX-FileCopyrightText: 2013-2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef FOLDERARCHIVEAGENTCHECKCOLLECTION_H
#define FOLDERARCHIVEAGENTCHECKCOLLECTION_H
#include <QObject>
#include <AkonadiCore/Collection>
#include <QDate>
class KJob;
class FolderArchiveAccountInfo;
class FolderArchiveAgentCheckCollection : public QObject
{
    Q_OBJECT
public:
    explicit FolderArchiveAgentCheckCollection(FolderArchiveAccountInfo *info, QObject *parent = nullptr);
    ~FolderArchiveAgentCheckCollection();

    void start();

Q_SIGNALS:
    void collectionIdFound(const Akonadi::Collection &col);
    void checkFailed(const QString &message);

private:
    Q_DISABLE_COPY(FolderArchiveAgentCheckCollection)
    void slotInitialCollectionFetchingFirstLevelDone(KJob *job);
    void slotCreateNewFolder(KJob *);
    void createNewFolder(const QString &name);
    QDate mCurrentDate;
    FolderArchiveAccountInfo *const mInfo;
};

#endif // FOLDERARCHIVEAGENTCHECKCOLLECTION_H
