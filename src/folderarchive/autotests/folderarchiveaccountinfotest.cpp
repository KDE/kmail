/*
   SPDX-FileCopyrightText: 2014-2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "folderarchiveaccountinfotest.h"
#include "../folderarchiveaccountinfo.h"
#include <Akonadi/Collection>
#include <KSharedConfig>
#include <QStandardPaths>
#include <QTest>
using namespace Qt::Literals::StringLiterals;

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
    QCOMPARE(info.folderArchiveType(), FolderArchiveAccountInfo::FolderArchiveType::UniqueFolder);
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
    info.setInstanceName(u"FOO"_s);
    QVERIFY(info.isValid());
}

void FolderArchiveAccountInfoTest::shouldRestoreFromSettings()
{
    FolderArchiveAccountInfo info;
    info.setInstanceName(u"FOO1"_s);
    info.setArchiveTopLevel(Akonadi::Collection(42).id());
    info.setFolderArchiveType(FolderArchiveAccountInfo::FolderArchiveType::FolderByMonths);
    info.setEnabled(true);
    info.setKeepExistingStructure(true);

    KConfigGroup grp(KSharedConfig::openConfig(), u"testsettings"_s);
    info.writeConfig(grp);

    FolderArchiveAccountInfo restoreInfo(grp);
    QCOMPARE(info, restoreInfo);
}

#include "moc_folderarchiveaccountinfotest.cpp"
