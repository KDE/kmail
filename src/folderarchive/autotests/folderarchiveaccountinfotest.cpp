/*
   SPDX-FileCopyrightText: 2014-2024 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "folderarchiveaccountinfotest.h"
#include "../folderarchiveaccountinfo.h"
#include <Akonadi/Collection>
#include <KSharedConfig>
#include <QStandardPaths>
#include <QTest>
QTEST_MAIN(FolderArchiveAccountInfoTest)

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
    QVERIFY(!info.enabled());
    QVERIFY(!info.keepExistingStructure());
    QVERIFY(!info.isValid());
    QVERIFY(!info.useDateFromMessage());
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

    KConfigGroup grp(KSharedConfig::openConfig(), QStringLiteral("testsettings"));
    info.writeConfig(grp);

    FolderArchiveAccountInfo restoreInfo(grp);
    QCOMPARE(info, restoreInfo);
}

#include "moc_folderarchiveaccountinfotest.cpp"
