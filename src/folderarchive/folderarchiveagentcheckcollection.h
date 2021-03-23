/*
   SPDX-FileCopyrightText: 2013-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once
#include <AkonadiCore/Collection>
#include <QDate>
#include <QObject>
class KJob;
class FolderArchiveAccountInfo;
class FolderArchiveAgentCheckCollection : public QObject
{
    Q_OBJECT
public:
    explicit FolderArchiveAgentCheckCollection(FolderArchiveAccountInfo *info, QObject *parent = nullptr);
    ~FolderArchiveAgentCheckCollection() override;

    void start();

Q_SIGNALS:
    void collectionIdFound(const Akonadi::Collection &col);
    void checkFailed(const QString &message);

private:
    Q_DISABLE_COPY(FolderArchiveAgentCheckCollection)
    void slotInitialCollectionFetchingFirstLevelDone(KJob *job);
    void slotCreateNewFolder(KJob *);
    void createNewFolder(const QString &name);
    const QDate mCurrentDate;
    FolderArchiveAccountInfo *const mInfo;
};

