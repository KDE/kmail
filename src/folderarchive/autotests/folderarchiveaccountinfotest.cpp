/*
   SPDX-FileCopyrightText: 2014-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "folderarchiveaccountinfotest.h"
#include "../folderarchiveaccountinfo.h"
#include <AkonadiCore/Collection>
#include <KSharedConfig>
#include <QStandardPaths>
#include <QTest>

FolderArchiveAccountInfoTest::FolderArchiveAccountInfoTest(QObject *parent)
    : QObject(parent)
{
    QStandardPaths::setTestModeEnabled(true);
}

FolderArchiveAccountInfoTest::~FolderArchiveAccountInfoTest() = default;

void FolderArchiveAccountInfoTest::shouldHaveDefaultValue()
{
    FolderArchiveAccountInfo info;
    QVERIFY(info.instanceName().isEmpty());
    QCOMPARE(info.archiveTopLevel(), Akonadi::Collection(-1).id());
    QCOMPARE(info.folderArchiveType(), FolderArchiveAccountInfo::UniqueFolder);
    QCOMPARE(info.enabled(), false);
    QCOMPARE(info.keepExistingStructure(), false);
    QCOMPARE(info.isValid(), false);
}

void FolderArchiveAccountInfoTest::shouldBeValid()
{
    FolderArchiveAccountInfo info;
    QVERIFY(!info.isValid());
    info.setArchiveTopLevel(Akonadi::Collection(42).id());
    QVERIFY(!info.isValid());
    info.setInstanceName(QStringLiteral("FOO"));
    QVERIFY(info.isValid());
}

void FolderArchiveAccountInfoTest::shouldRestoreFromSettings()
{
    FolderArchiveAccountInfo info;
    info.setInstanceName(QStringLiteral("FOO1"));
    info.setArchiveTopLevel(Akonadi::Collection(42).id());
    info.setFolderArchiveType(FolderArchiveAccountInfo::FolderByMonths);
    info.setEnabled(true);
    info.setKeepExistingStructure(true);

    KConfigGroup grp(KSharedConfig::openConfig(), "testsettings");
    info.writeConfig(grp);

    FolderArchiveAccountInfo restoreInfo(grp);
    QCOMPARE(info, restoreInfo);
}

QTEST_MAIN(FolderArchiveAccountInfoTest)
