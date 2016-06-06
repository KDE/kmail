/*
   Copyright (C) 2014-2016 Montel Laurent <montel@kde.org>

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

#include "sendlaterconfigtest.h"
#include "sendlaterinfo.h"
#include "sendlaterutil.h"
#include <qtest.h>

SendLaterConfigTest::SendLaterConfigTest(QObject *parent)
    : QObject(parent)
{

}

SendLaterConfigTest::~SendLaterConfigTest()
{

}

void SendLaterConfigTest::init()
{
    mConfig = KSharedConfig::openConfig(QStringLiteral("test-sendlateragent.rc"), KConfig::SimpleConfig);
    mSendlaterRegExpFilter = QRegularExpression(QStringLiteral("SendLaterItem \\d+"));
    cleanup();
}

void SendLaterConfigTest::cleanup()
{
    const QStringList filterGroups = mConfig->groupList();
    foreach (const QString &group, filterGroups) {
        mConfig->deleteGroup(group);
    }
    mConfig->sync();
    mConfig->reparseConfiguration();
}

void SendLaterConfigTest::cleanupTestCase()
{
    //Make sure to clean config
    cleanup();
}

void SendLaterConfigTest::shouldConfigBeEmpty()
{
    const QStringList filterGroups = mConfig->groupList();
    QCOMPARE(filterGroups.isEmpty(), true);
}

void SendLaterConfigTest::shouldAddAnItem()
{
    SendLater::SendLaterInfo info;
    const QString to = QStringLiteral("kde.org");
    info.setTo(to);
    info.setItemId(Akonadi::Item::Id(42));
    info.setSubject(QStringLiteral("Subject"));
    info.setRecurrence(true);
    info.setRecurrenceEachValue(5);
    info.setRecurrenceUnit(SendLater::SendLaterInfo::Years);
    const QDate date(2014, 1, 1);
    info.setDateTime(QDateTime(date));
    info.setLastDateTimeSend(QDateTime(date));

    SendLater::SendLaterUtil::writeSendLaterInfo(mConfig, &info, false);
    const QStringList itemList = mConfig->groupList().filter(mSendlaterRegExpFilter);

    QCOMPARE(itemList.isEmpty(), false);
    QCOMPARE(itemList.count(), 1);
}

void SendLaterConfigTest::shouldNotAddInvalidItem()
{
    SendLater::SendLaterInfo info;
    SendLater::SendLaterUtil::writeSendLaterInfo(mConfig, &info, false);
    const QStringList itemList = mConfig->groupList().filter(mSendlaterRegExpFilter);

    QCOMPARE(itemList.isEmpty(), true);
}

QTEST_MAIN(SendLaterConfigTest)
