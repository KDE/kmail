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

#ifndef FOLDERARCHIVEACCOUNTINFO_H
#define FOLDERARCHIVEACCOUNTINFO_H

#include <KConfigGroup>
#include <Akonadi/Collection>

class FolderArchiveAccountInfo
{
public:
    FolderArchiveAccountInfo();
    explicit FolderArchiveAccountInfo(const KConfigGroup &config);
    ~FolderArchiveAccountInfo();

    enum FolderArchiveType {
        UniqueFolder,
        FolderByMonths,
        FolderByYears
    };

    bool isValid() const;

    QString instanceName() const;
    void setInstanceName(const QString &instance);

    void setArchiveTopLevel(Akonadi::Collection::Id id);
    Akonadi::Collection::Id archiveTopLevel() const;

    void setFolderArchiveType(FolderArchiveType type);
    FolderArchiveType folderArchiveType() const;

    void setEnabled(bool enabled);
    bool enabled() const;

    void setKeepExistingStructure(bool b);
    bool keepExistingStructure() const;

    void writeConfig(KConfigGroup &config );
    void readConfig(const KConfigGroup &config);

private:
    FolderArchiveAccountInfo::FolderArchiveType mArchiveType;
    Akonadi::Collection::Id mArchiveTopLevelCollectionId;
    QString mInstanceName;
    bool mEnabled;
    bool mKeepExistingStructure;
};

#endif // FOLDERARCHIVEACCOUNTINFO_H
