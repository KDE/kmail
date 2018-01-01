/*
   Copyright (C) 2014-2018 Montel Laurent <montel@kde.org>

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

#include "archivemailinfotest.h"
#include "../archivemailinfo.h"
#include <AkonadiCore/Collection>
#include <qtest.h>
#include <KConfigGroup>
#include <KSharedConfig>
#include <QStandardPaths>

ArchiveMailInfoTest::ArchiveMailInfoTest(QObject *parent)
    : QObject(parent)
{
    QStandardPaths::setTestModeEnabled(true);
}

ArchiveMailInfoTest::~ArchiveMailInfoTest()
{
}

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
