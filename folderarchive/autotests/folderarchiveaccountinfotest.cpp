/*
  Copyright (c) 2014 Montel Laurent <montel@kde.org>

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

#include "folderarchiveaccountinfotest.h"
#include "../folderarchiveaccountinfo.h"
#include <AkonadiCore/Collection>
#include <qtest.h>
#include <KSharedConfig>

FolderArchiveAccountInfoTest::FolderArchiveAccountInfoTest(QObject *parent)
    : QObject(parent)
{

}

FolderArchiveAccountInfoTest::~FolderArchiveAccountInfoTest()
{

}

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
