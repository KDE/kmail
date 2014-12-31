/*
  Copyright (c) 2013-2015 Montel Laurent <montel@kde.org>

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

#include "folderarchiveaccountinfo.h"

#include <KConfigGroup>

FolderArchiveAccountInfo::FolderArchiveAccountInfo()
    : mArchiveType(UniqueFolder),
      mArchiveTopLevelCollectionId(-1),
      mEnabled(false),
      mKeepExistingStructure(false)
{
}

FolderArchiveAccountInfo::FolderArchiveAccountInfo(const KConfigGroup &config)
    : mArchiveType(UniqueFolder),
      mArchiveTopLevelCollectionId(-1),
      mEnabled(false),
      mKeepExistingStructure(false)
{
    readConfig(config);
}

FolderArchiveAccountInfo::~FolderArchiveAccountInfo()
{
}

bool FolderArchiveAccountInfo::isValid() const
{
    return (mArchiveTopLevelCollectionId > -1) && (!mInstanceName.isEmpty());
}

void FolderArchiveAccountInfo::setFolderArchiveType(FolderArchiveAccountInfo::FolderArchiveType type)
{
    mArchiveType = type;
}

FolderArchiveAccountInfo::FolderArchiveType FolderArchiveAccountInfo::folderArchiveType() const
{
    return mArchiveType;
}

void FolderArchiveAccountInfo::setArchiveTopLevel(Akonadi::Collection::Id id)
{
    mArchiveTopLevelCollectionId = id;
}

Akonadi::Collection::Id FolderArchiveAccountInfo::archiveTopLevel() const
{
    return mArchiveTopLevelCollectionId;
}

QString FolderArchiveAccountInfo::instanceName() const
{
    return mInstanceName;
}

void FolderArchiveAccountInfo::setInstanceName(const QString &instance)
{
    mInstanceName = instance;
}

void FolderArchiveAccountInfo::setEnabled(bool enabled)
{
    mEnabled = enabled;
}

bool FolderArchiveAccountInfo::enabled() const
{
    return mEnabled;
}

void FolderArchiveAccountInfo::setKeepExistingStructure(bool b)
{
    mKeepExistingStructure = b;
}

bool FolderArchiveAccountInfo::keepExistingStructure() const
{
    return mKeepExistingStructure;
}

void FolderArchiveAccountInfo::readConfig(const KConfigGroup &config)
{
    mInstanceName = config.readEntry(QLatin1String("instanceName"));
    mArchiveTopLevelCollectionId = config.readEntry(QLatin1String("topLevelCollectionId"), -1);
    mArchiveType = static_cast<FolderArchiveType>(config.readEntry("folderArchiveType", (int)UniqueFolder));
    mEnabled = config.readEntry("enabled", false);
    mKeepExistingStructure = config.readEntry("keepExistingStructure", false);
}

void FolderArchiveAccountInfo::writeConfig(KConfigGroup &config)
{
    config.writeEntry(QLatin1String("instanceName"), mInstanceName);
    if (mArchiveTopLevelCollectionId > -1) {
        config.writeEntry(QLatin1String("topLevelCollectionId"), mArchiveTopLevelCollectionId);
    } else {
        config.deleteEntry(QLatin1String("topLevelCollectionId"));
    }

    config.writeEntry(QLatin1String("folderArchiveType"), (int)mArchiveType);
    config.writeEntry(QLatin1String("enabled"), mEnabled);
    config.writeEntry("keepExistingStructure", mKeepExistingStructure);
}

bool FolderArchiveAccountInfo::operator==(const FolderArchiveAccountInfo &other) const
{
    return (mInstanceName == other.instanceName())
           && (mArchiveTopLevelCollectionId == other.archiveTopLevel())
           && (mArchiveType == other.folderArchiveType())
           && (mEnabled == other.enabled())
           && (mKeepExistingStructure == other.keepExistingStructure());
}
