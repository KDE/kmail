/*
   SPDX-FileCopyrightText: 2014-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "archivemailinfotest.h"
#include "../archivemailinfo.h"
#include <AkonadiCore/Collection>
#include <KConfigGroup>
#include <KSharedConfig>
#include <QStandardPaths>
#include <QTest>

ArchiveMailInfoTest::ArchiveMailInfoTest(QObject *parent)
    : QObject(parent)
{
    QStandardPaths::setTestModeEnabled(true);
}

ArchiveMailInfoTest::~ArchiveMailInfoTest() = default;

void ArchiveMailInfoTest::shouldHaveDefaultValue()
{
    ArchiveMailInfo info;
    QCOMPARE(info.saveCollectionId(), Akonadi::Collection::Id(-1));
    QCOMPARE(info.saveSubCollection(), false);
    QCOMPARE(info.url(), QUrl());
    QCOMPARE(info.archiveType(), MailCommon::BackupJob::Zip);
    QCOMPARE(info.archiveUnit(), ArchiveMailInfo::ArchiveDays);
    QCOMPARE(info.archiveAge(), 1);
    QCOMPARE(info.lastDateSaved(), QDate());
    QCOMPARE(info.maximumArchiveCount(), 0);
    QCOMPARE(info.isEnabled(), true);
}

void ArchiveMailInfoTest::shouldRestoreFromSettings()
{
    ArchiveMailInfo info;
    info.setSaveCollectionId(Akonadi::Collection::Id(42));
    info.setUrl(QUrl::fromLocalFile(QStringLiteral("/foo/foo")));
    info.setArchiveType(MailCommon::BackupJob::TarBz2);
    info.setArchiveUnit(ArchiveMailInfo::ArchiveMonths);
    info.setArchiveAge(5);
    info.setLastDateSaved(QDate::currentDate());
    info.setMaximumArchiveCount(5);
    info.setEnabled(false);

    KConfigGroup grp(KSharedConfig::openConfig(), "testsettings");
    info.writeConfig(grp);

    ArchiveMailInfo restoreInfo(grp);
    QCOMPARE(info, restoreInfo);
}

void ArchiveMailInfoTest::shouldCopyArchiveInfo()
{
    ArchiveMailInfo info;
    info.setSaveCollectionId(Akonadi::Collection::Id(42));
    info.setUrl(QUrl::fromLocalFile(QStringLiteral("/foo/foo")));
    info.setArchiveType(MailCommon::BackupJob::TarBz2);
    info.setArchiveUnit(ArchiveMailInfo::ArchiveMonths);
    info.setArchiveAge(5);
    info.setLastDateSaved(QDate::currentDate());
    info.setMaximumArchiveCount(5);
    info.setEnabled(false);

    ArchiveMailInfo copyInfo(info);
    QCOMPARE(info, copyInfo);
}

QTEST_MAIN(ArchiveMailInfoTest)
