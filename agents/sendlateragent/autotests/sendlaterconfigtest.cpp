/*
   SPDX-FileCopyrightText: 2014-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "sendlaterconfigtest.h"
#include "sendlaterutil.h"

#include <MessageComposer/SendLaterInfo>

#include <QTest>

SendLaterConfigTest::SendLaterConfigTest(QObject *parent)
    : QObject(parent)
{
}

SendLaterConfigTest::~SendLaterConfigTest() = default;

void SendLaterConfigTest::init()
{
    mConfig = KSharedConfig::openConfig(QStringLiteral("test-sendlateragent.rc"), KConfig::SimpleConfig);
    mSendlaterRegExpFilter = QRegularExpression(QStringLiteral("SendLaterItem \\d+"));
    cleanup();
}

void SendLaterConfigTest::cleanup()
{
    const QStringList filterGroups = mConfig->groupList();
    for (const QString &group : filterGroups) {
        mConfig->deleteGroup(group);
    }
    mConfig->sync();
    mConfig->reparseConfiguration();
}

void SendLaterConfigTest::cleanupTestCase()
{
    // Make sure to clean config
    cleanup();
}

void SendLaterConfigTest::shouldConfigBeEmpty()
{
    const QStringList filterGroups = mConfig->groupList();
    QCOMPARE(filterGroups.isEmpty(), true);
}

void SendLaterConfigTest::shouldAddAnItem()
{
    MessageComposer::SendLaterInfo info;
    const QString to = QStringLiteral("kde.org");
    info.setTo(to);
    info.setItemId(Akonadi::Item::Id(42));
    info.setSubject(QStringLiteral("Subject"));
    info.setRecurrence(true);
    info.setRecurrenceEachValue(5);
    info.setRecurrenceUnit(MessageComposer::SendLaterInfo::Years);
    const QDate date(2014, 1, 1);
    info.setDateTime(QDateTime(date.startOfDay()));
    info.setLastDateTimeSend(QDateTime(date.startOfDay()));

    SendLaterUtil::writeSendLaterInfo(mConfig, &info);
    const QStringList itemList = mConfig->groupList().filter(mSendlaterRegExpFilter);

    QCOMPARE(itemList.isEmpty(), false);
    QCOMPARE(itemList.count(), 1);
}

void SendLaterConfigTest::shouldNotAddInvalidItem()
{
    MessageComposer::SendLaterInfo info;
    SendLaterUtil::writeSendLaterInfo(mConfig, &info);
    const QStringList itemList = mConfig->groupList().filter(mSendlaterRegExpFilter);

    QCOMPARE(itemList.isEmpty(), true);
}

QTEST_MAIN(SendLaterConfigTest)
