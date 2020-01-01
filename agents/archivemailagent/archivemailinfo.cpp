/*
   Copyright (C) 2012-2020 Laurent Montel <montel@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/
#include "archivemailinfo.h"

#include <KLocalizedString>
#include "archivemailagent_debug.h"
#include <QDir>

ArchiveMailInfo::ArchiveMailInfo()
{
}

ArchiveMailInfo::ArchiveMailInfo(const KConfigGroup &config)
{
    readConfig(config);
}

ArchiveMailInfo::ArchiveMailInfo(const ArchiveMailInfo &info)
{
    mLastDateSaved = info.lastDateSaved();
    mArchiveAge = info.archiveAge();
    mArchiveType = info.archiveType();
    mArchiveUnit = info.archiveUnit();
    mSaveCollectionId = info.saveCollectionId();
    mMaximumArchiveCount = info.maximumArchiveCount();
    mSaveSubCollection = info.saveSubCollection();
    mPath = info.url();
    mIsEnabled = info.isEnabled();
}

ArchiveMailInfo::~ArchiveMailInfo()
{
}

ArchiveMailInfo &ArchiveMailInfo::operator=(const ArchiveMailInfo &old)
{
    mLastDateSaved = old.lastDateSaved();
    mArchiveAge = old.archiveAge();
    mArchiveType = old.archiveType();
    mArchiveUnit = old.archiveUnit();
    mSaveCollectionId = old.saveCollectionId();
    mMaximumArchiveCount = old.maximumArchiveCount();
    mSaveSubCollection = old.saveSubCollection();
    mPath = old.url();
    mIsEnabled = old.isEnabled();
    return *this;
}

QString normalizeFolderName(const QString &folderName)
{
    QString adaptFolderName(folderName);
    adaptFolderName.replace(QLatin1Char('/'), QLatin1Char('_'));
    return adaptFolderName;
}

QString ArchiveMailInfo::dirArchive(bool &dirExit) const
{
    const QDir dir(url().path());
    QString dirPath = url().path();
    if (!dir.exists()) {
        dirExit = false;
        dirPath = QDir::homePath();
        qCDebug(ARCHIVEMAILAGENT_LOG) << " Path doesn't exist" << dir.path();
    } else {
        dirExit = true;
    }
    return dirPath;
}

QUrl ArchiveMailInfo::realUrl(const QString &folderName, bool &dirExist) const
{
    const int numExtensions = 4;
    // The extensions here are also sorted, like the enum order of BackupJob::ArchiveType
    const char *extensions[numExtensions] = { ".zip", ".tar", ".tar.bz2", ".tar.gz" };
    const QString dirPath = dirArchive(dirExist);

    const QString path = dirPath + QLatin1Char('/') + i18nc("Start of the filename for a mail archive file", "Archive")
                         + QLatin1Char('_') + normalizeFolderName(folderName) + QLatin1Char('_')
                         + QDate::currentDate().toString(Qt::ISODate) + QString::fromLatin1(extensions[mArchiveType]);
    QUrl real(QUrl::fromLocalFile(path));
    return real;
}

QStringList ArchiveMailInfo::listOfArchive(const QString &folderName, bool &dirExist) const
{
    const int numExtensions = 4;
    // The extensions here are also sorted, like the enum order of BackupJob::ArchiveType
    const char *extensions[numExtensions] = { ".zip", ".tar", ".tar.bz2", ".tar.gz" };
    const QString dirPath = dirArchive(dirExist);

    QDir dir(dirPath);

    QStringList nameFilters;
    nameFilters << i18nc("Start of the filename for a mail archive file", "Archive") + QLatin1Char('_')
        +normalizeFolderName(folderName) + QLatin1Char('_') + QLatin1String("*") + QString::fromLatin1(extensions[mArchiveType]);
    const QStringList lst = dir.entryList(nameFilters, QDir::Files | QDir::NoDotAndDotDot, QDir::Time | QDir::Reversed);
    return lst;
}

bool ArchiveMailInfo::isValid() const
{
    return mSaveCollectionId != -1;
}

void ArchiveMailInfo::setArchiveAge(int age)
{
    mArchiveAge = age;
}

int ArchiveMailInfo::archiveAge() const
{
    return mArchiveAge;
}

void ArchiveMailInfo::setArchiveUnit(ArchiveMailInfo::ArchiveUnit unit)
{
    mArchiveUnit = unit;
}

ArchiveMailInfo::ArchiveUnit ArchiveMailInfo::archiveUnit() const
{
    return mArchiveUnit;
}

void ArchiveMailInfo::setArchiveType(MailCommon::BackupJob::ArchiveType type)
{
    mArchiveType = type;
}

MailCommon::BackupJob::ArchiveType ArchiveMailInfo::archiveType() const
{
    return mArchiveType;
}

void ArchiveMailInfo::setLastDateSaved(const QDate &date)
{
    mLastDateSaved = date;
}

QDate ArchiveMailInfo::lastDateSaved() const
{
    return mLastDateSaved;
}

void ArchiveMailInfo::readConfig(const KConfigGroup &config)
{
    mPath = QUrl::fromUserInput(config.readEntry("storePath"));

    if (config.hasKey(QStringLiteral("lastDateSaved"))) {
        mLastDateSaved = QDate::fromString(config.readEntry("lastDateSaved"), Qt::ISODate);
    }
    mSaveSubCollection = config.readEntry("saveSubCollection", false);
    mArchiveType = static_cast<MailCommon::BackupJob::ArchiveType>(config.readEntry("archiveType", (int)MailCommon::BackupJob::Zip));
    mArchiveUnit = static_cast<ArchiveUnit>(config.readEntry("archiveUnit", (int)ArchiveDays));
    Akonadi::Collection::Id tId = config.readEntry("saveCollectionId", mSaveCollectionId);
    mArchiveAge = config.readEntry("archiveAge", 1);
    mMaximumArchiveCount = config.readEntry("maximumArchiveCount", 0);
    if (tId >= 0) {
        mSaveCollectionId = tId;
    }
    mIsEnabled = config.readEntry("enabled", true);
}

void ArchiveMailInfo::writeConfig(KConfigGroup &config)
{
    if (!isValid()) {
        return;
    }
    config.writeEntry("storePath", mPath.toLocalFile());

    if (mLastDateSaved.isValid()) {
        config.writeEntry("lastDateSaved", mLastDateSaved.toString(Qt::ISODate));
    }

    config.writeEntry("saveSubCollection", mSaveSubCollection);
    config.writeEntry("archiveType", static_cast<int>(mArchiveType));
    config.writeEntry("archiveUnit", static_cast<int>(mArchiveUnit));
    config.writeEntry("saveCollectionId", mSaveCollectionId);
    config.writeEntry("archiveAge", mArchiveAge);
    config.writeEntry("maximumArchiveCount", mMaximumArchiveCount);
    config.writeEntry("enabled", mIsEnabled);
    config.sync();
}

QUrl ArchiveMailInfo::url() const
{
    return mPath;
}

void ArchiveMailInfo::setUrl(const QUrl &url)
{
    mPath = url;
}

bool ArchiveMailInfo::saveSubCollection() const
{
    return mSaveSubCollection;
}

void ArchiveMailInfo::setSaveSubCollection(bool saveSubCol)
{
    mSaveSubCollection = saveSubCol;
}

void ArchiveMailInfo::setSaveCollectionId(Akonadi::Collection::Id collectionId)
{
    mSaveCollectionId = collectionId;
}

Akonadi::Collection::Id ArchiveMailInfo::saveCollectionId() const
{
    return mSaveCollectionId;
}

int ArchiveMailInfo::maximumArchiveCount() const
{
    return mMaximumArchiveCount;
}

void ArchiveMailInfo::setMaximumArchiveCount(int max)
{
    mMaximumArchiveCount = max;
}

bool ArchiveMailInfo::isEnabled() const
{
    return mIsEnabled;
}

void ArchiveMailInfo::setEnabled(bool b)
{
    mIsEnabled = b;
}

bool ArchiveMailInfo::operator==(const ArchiveMailInfo &other) const
{
    return saveCollectionId() == other.saveCollectionId()
           && saveSubCollection() == other.saveSubCollection()
           && url() == other.url()
           && archiveType() == other.archiveType()
           && archiveUnit() == other.archiveUnit()
           && archiveAge() == other.archiveAge()
           && lastDateSaved() == other.lastDateSaved()
           && maximumArchiveCount() == other.maximumArchiveCount()
           && isEnabled() == other.isEnabled();
}
