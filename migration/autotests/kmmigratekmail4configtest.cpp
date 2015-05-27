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

#include "kmmigratekmail4configtest.h"
#include "../kmmigratekmail4config.h"
#include <qtest.h>

KMMigrateKMail4ConfigTest::KMMigrateKMail4ConfigTest(QObject *parent)
    : QObject(parent)
{

}

KMMigrateKMail4ConfigTest::~KMMigrateKMail4ConfigTest()
{

}

void KMMigrateKMail4ConfigTest::shouldHaveDefaultValue()
{
    KMMigrateKMail4Config migrate;
    QVERIFY(!migrate.start());
    QVERIFY(migrate.configFileName().isEmpty());
}

void KMMigrateKMail4ConfigTest::shouldVerifyIfCheckIsNecessary()
{
    KMMigrateKMail4Config migrate;
    //Invalid before config file is not set.
    QVERIFY(!migrate.checkIfNecessary());
}

QTEST_MAIN(KMMigrateKMail4ConfigTest)
