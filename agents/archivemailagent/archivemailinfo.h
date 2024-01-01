/*
   SPDX-FileCopyrightText: 2012-2024 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <Akonadi/Collection>
#include <KConfigGroup>
#include <MailCommon/BackupJob>
#include <QDate>
#include <QUrl>

class ArchiveMailInfo
{
public:
    explicit ArchiveMailInfo(const KConfigGroup &config);
    ArchiveMailInfo(const ArchiveMailInfo &info);
    ArchiveMailInfo();

    ~ArchiveMailInfo();

    ArchiveMailInfo &operator=(const ArchiveMailInfo &old);

    enum ArchiveUnit {
        ArchiveDays = 0,
        ArchiveWeeks,
        ArchiveMonths,
        ArchiveYears,
    };

    [[nodiscard]] QUrl realUrl(const QString &folderName, bool &dirExist) const;

    [[nodiscard]] bool isValid() const;

    [[nodiscard]] Akonadi::Collection::Id saveCollectionId() const;
    void setSaveCollectionId(Akonadi::Collection::Id collectionId);

    void setSaveSubCollection(bool b);
    [[nodiscard]] bool saveSubCollection() const;

    void setUrl(const QUrl &url);
    [[nodiscard]] QUrl url() const;

    void readConfig(const KConfigGroup &config);
    void writeConfig(KConfigGroup &config);

    void setArchiveType(MailCommon::BackupJob::ArchiveType type);
    [[nodiscard]] MailCommon::BackupJob::ArchiveType archiveType() const;

    void setArchiveUnit(ArchiveMailInfo::ArchiveUnit unit);
    [[nodiscard]] ArchiveMailInfo::ArchiveUnit archiveUnit() const;

    void setArchiveAge(int age);
    [[nodiscard]] int archiveAge() const;

    void setLastDateSaved(QDate date);
    [[nodiscard]] QDate lastDateSaved() const;

    [[nodiscard]] int maximumArchiveCount() const;
    void setMaximumArchiveCount(int max);

    [[nodiscard]] QStringList listOfArchive(const QString &foldername, bool &dirExist) const;

    [[nodiscard]] bool isEnabled() const;
    void setEnabled(bool b);

    [[nodiscard]] bool operator==(const ArchiveMailInfo &other) const;

    [[nodiscard]] bool useRange() const;
    void setUseRange(bool newUseRange);

    [[nodiscard]] QList<int> range() const;
    void setRange(const QList<int> &newRanges);

private:
    [[nodiscard]] QString dirArchive(bool &dirExit) const;
    QDate mLastDateSaved;
    int mArchiveAge = 1;
    MailCommon::BackupJob::ArchiveType mArchiveType = MailCommon::BackupJob::Zip;
    ArchiveUnit mArchiveUnit = ArchiveMailInfo::ArchiveDays;
    Akonadi::Collection::Id mSaveCollectionId = -1;
    QUrl mPath;
    QList<int> mRanges;
    int mMaximumArchiveCount = 0;
    bool mSaveSubCollection = false;
    bool mIsEnabled = true;
    bool mUseRange = false;
};
