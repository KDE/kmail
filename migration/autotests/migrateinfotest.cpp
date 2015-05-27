/*
  Copyright (c) 2015 Montel Laurent <montel@kde.org>

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

#include "migrateinfotest.h"
#include "../migrateinfo.h"
#include <qtest.h>

MigrateInfoTest::MigrateInfoTest(QObject *parent)
    : QObject(parent)
{

}

MigrateInfoTest::~MigrateInfoTest()
{

}

void MigrateInfoTest::shouldHaveDefaultValue()
{
    MigrateInfo info;
    QVERIFY(!info.folder());
    QVERIFY(info.path().isEmpty());
    QVERIFY(info.type().isEmpty());
}

void MigrateInfoTest::shouldAssignValue()
{
    MigrateInfo info;
    bool isFolder = false;
    info.setFolder(isFolder);
    QCOMPARE(info.folder(), isFolder);
    isFolder = true;
    info.setFolder(isFolder);
    QCOMPARE(info.folder(), isFolder);
    isFolder = false;
    info.setFolder(isFolder);
    QCOMPARE(info.folder(), isFolder);

    QString path;
    info.setPath(path);
    QCOMPARE(info.path(), path);
    path = QStringLiteral("foo");
    info.setPath(path);
    QCOMPARE(info.path(), path);
    path.clear();
    info.setPath(path);
    QCOMPARE(info.path(), path);

    QString type;
    info.setType(type);
    QCOMPARE(info.type(), type);
    type = QStringLiteral("foo");
    info.setType(type);
    QCOMPARE(info.type(), type);
    type.clear();
    info.setType(type);
    QCOMPARE(info.type(), type);
}

void MigrateInfoTest::shouldBeEmpty()
{
    MigrateInfo info;
    QVERIFY(!info.isValid());
    info.setFolder(true);
    QVERIFY(!info.isValid());

    const QString type = QStringLiteral("foo");
    info.setType(type);
    QVERIFY(!info.isValid());

    QString path = QStringLiteral("foo");
    info.setPath(path);
    QVERIFY(info.isValid());

    path.clear();
    info.setPath(path);
    QVERIFY(!info.isValid());
}

QTEST_MAIN(MigrateInfoTest)
