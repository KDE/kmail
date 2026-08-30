/*
   SPDX-FileCopyrightText: 2013-2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "folderarchiveaccountinfo.h"
using namespace Qt::Literals::StringLiterals;

FolderArchiveAccountInfo::FolderArchiveAccountInfo() = default;

FolderArchiveAccountInfo::FolderArchiveAccountInfo(const KConfigGroup &config)
{
    readConfig(config);
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
    mInstanceName = config.readEntry(u"instanceName"_s);
    mArchiveTopLevelCollectionId = config.readEntry(u"topLevelCollectionId"_s, -1);
    mArchiveType = static_cast<FolderArchiveType>(config.readEntry("folderArchiveType", (int)FolderArchiveAccountInfo::FolderArchiveType::UniqueFolder));
    mEnabled = config.readEntry("enabled", false);
    mKeepExistingStructure = config.readEntry("keepExistingStructure", false);
    mUseDateFromMessage = config.readEntry("useDateFromMessage", false);
}

void FolderArchiveAccountInfo::writeConfig(KConfigGroup &config)
{
    config.writeEntry(u"instanceName"_s, mInstanceName);
    if (mArchiveTopLevelCollectionId > -1) {
        config.writeEntry(u"topLevelCollectionId"_s, mArchiveTopLevelCollectionId);
    } else {
        config.deleteEntry(u"topLevelCollectionId"_s);
    }

    config.writeEntry(u"folderArchiveType"_s, (int)mArchiveType);
    config.writeEntry(u"enabled"_s, mEnabled);
    config.writeEntry("keepExistingStructure", mKeepExistingStructure);
    config.writeEntry("useDateFromMessage", mUseDateFromMessage);
}

bool FolderArchiveAccountInfo::operator==(const FolderArchiveAccountInfo &other) const
{
    return (mInstanceName == other.instanceName()) && (mArchiveTopLevelCollectionId == other.archiveTopLevel()) && (mArchiveType == other.folderArchiveType())
        && (mEnabled == other.enabled()) && (mKeepExistingStructure == other.keepExistingStructure()) && (mUseDateFromMessage == other.useDateFromMessage());
}

bool FolderArchiveAccountInfo::useDateFromMessage() const
{
    return mUseDateFromMessage;
}

void FolderArchiveAccountInfo::setUseDateFromMessage(bool newUseDateFromMessage)
{
    mUseDateFromMessage = newUseDateFromMessage;
}
