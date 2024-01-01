/*
   SPDX-FileCopyrightText: 2012-2024 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QObject>

#include <Akonadi/Collection>
#include <KSharedConfig>

class ArchiveMailKernel;
class ArchiveMailInfo;

class ArchiveMailManager : public QObject
{
    Q_OBJECT
public:
    explicit ArchiveMailManager(QObject *parent = nullptr);
    ~ArchiveMailManager() override;
    void removeCollection(const Akonadi::Collection &collection);
    void backupDone(ArchiveMailInfo *info);
    void pause();
    void resume();

    [[nodiscard]] QString printArchiveListInfo() const;
    void collectionDoesntExist(ArchiveMailInfo *info);

    [[nodiscard]] QString printCurrentListInfo() const;

    void archiveFolder(const QString &path, Akonadi::Collection::Id collectionId);

    ArchiveMailKernel *kernel() const;

public Q_SLOTS:
    void load();

Q_SIGNALS:
    void needUpdateConfigDialogBox();

private:
    [[nodiscard]] QString infoToStr(ArchiveMailInfo *info) const;
    void removeCollectionId(Akonadi::Collection::Id id);
    KSharedConfig::Ptr mConfig;
    QList<ArchiveMailInfo *> mListArchiveInfo;
    ArchiveMailKernel *mArchiveMailKernel = nullptr;
};
