/*
   SPDX-FileCopyrightText: 2013-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include "kmail_private_export.h"
#include <AkonadiCore/Collection>
#include <KConfigGroup>

class KMAILTESTS_TESTS_EXPORT FolderArchiveAccountInfo
{
public:
    FolderArchiveAccountInfo();
    FolderArchiveAccountInfo(const KConfigGroup &config);
    ~FolderArchiveAccountInfo();

    enum FolderArchiveType {
        UniqueFolder,
        FolderByMonths,
        FolderByYears,
    };

    Q_REQUIRED_RESULT bool isValid() const;

    Q_REQUIRED_RESULT QString instanceName() const;
    void setInstanceName(const QString &instance);

    void setArchiveTopLevel(Akonadi::Collection::Id id);
    Q_REQUIRED_RESULT Akonadi::Collection::Id archiveTopLevel() const;

    void setFolderArchiveType(FolderArchiveType type);
    Q_REQUIRED_RESULT FolderArchiveType folderArchiveType() const;

    void setEnabled(bool enabled);
    Q_REQUIRED_RESULT bool enabled() const;

    void setKeepExistingStructure(bool b);
    Q_REQUIRED_RESULT bool keepExistingStructure() const;

    void writeConfig(KConfigGroup &config);
    void readConfig(const KConfigGroup &config);

    Q_REQUIRED_RESULT bool operator==(const FolderArchiveAccountInfo &other) const;

private:
    Akonadi::Collection::Id mArchiveTopLevelCollectionId = -1;
    QString mInstanceName;
    FolderArchiveAccountInfo::FolderArchiveType mArchiveType = UniqueFolder;
    bool mEnabled = false;
    bool mKeepExistingStructure = false;
};

