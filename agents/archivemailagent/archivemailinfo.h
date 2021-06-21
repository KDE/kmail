/*
   SPDX-FileCopyrightText: 2012-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <Collection>
#include <KConfigGroup>
#include <MailCommon/BackupJob>
#include <QDate>
#include <QUrl>

class ArchiveMailInfo
{
public:
    explicit ArchiveMailInfo();
    explicit ArchiveMailInfo(const KConfigGroup &config);
    ArchiveMailInfo(const ArchiveMailInfo &info);
    ~ArchiveMailInfo();

    ArchiveMailInfo &operator=(const ArchiveMailInfo &old);

    enum ArchiveUnit {
        ArchiveDays = 0,
        ArchiveWeeks,
        ArchiveMonths,
        ArchiveYears,
    };

    Q_REQUIRED_RESULT QUrl realUrl(const QString &folderName, bool &dirExist) const;

    Q_REQUIRED_RESULT bool isValid() const;

    Q_REQUIRED_RESULT Akonadi::Collection::Id saveCollectionId() const;
    void setSaveCollectionId(Akonadi::Collection::Id collectionId);

    void setSaveSubCollection(bool b);
    Q_REQUIRED_RESULT bool saveSubCollection() const;

    void setUrl(const QUrl &url);
    Q_REQUIRED_RESULT QUrl url() const;

    void readConfig(const KConfigGroup &config);
    void writeConfig(KConfigGroup &config);

    void setArchiveType(MailCommon::BackupJob::ArchiveType type);
    Q_REQUIRED_RESULT MailCommon::BackupJob::ArchiveType archiveType() const;

    void setArchiveUnit(ArchiveMailInfo::ArchiveUnit unit);
    Q_REQUIRED_RESULT ArchiveMailInfo::ArchiveUnit archiveUnit() const;

    void setArchiveAge(int age);
    Q_REQUIRED_RESULT int archiveAge() const;

    void setLastDateSaved(QDate date);
    Q_REQUIRED_RESULT QDate lastDateSaved() const;

    Q_REQUIRED_RESULT int maximumArchiveCount() const;
    void setMaximumArchiveCount(int max);

    Q_REQUIRED_RESULT QStringList listOfArchive(const QString &foldername, bool &dirExist) const;

    Q_REQUIRED_RESULT bool isEnabled() const;
    void setEnabled(bool b);

    Q_REQUIRED_RESULT bool operator==(const ArchiveMailInfo &other) const;

private:
    QString dirArchive(bool &dirExit) const;
    QDate mLastDateSaved;
    int mArchiveAge = 1;
    MailCommon::BackupJob::ArchiveType mArchiveType = MailCommon::BackupJob::Zip;
    ArchiveUnit mArchiveUnit = ArchiveMailInfo::ArchiveDays;
    Akonadi::Collection::Id mSaveCollectionId = -1;
    QUrl mPath;
    int mMaximumArchiveCount = 0;
    bool mSaveSubCollection = false;
    bool mIsEnabled = true;
};

