/*
   SPDX-FileCopyrightText: 2014-2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "sendlaterconfigtest.h"
#include "sendlaterutil.h"

#include <MessageComposer/SendLaterInfo>

#include <QStandardPaths>
#include <QTest>
using namespace Qt::Literals::StringLiterals;

SendLaterConfigTest::SendLaterConfigTest(QObject *parent)
    : QObject(parent)
{
    QStandardPaths::setTestModeEnabled(true);
}

SendLaterConfigTest::~SendLaterConfigTest() = default;

void SendLaterConfigTest::init()
{
    mConfig = KSharedConfig::openConfig(u"test-sendlateragent.rc"_s, KConfig::SimpleConfig);
    mSendlaterRegExpFilter = QRegularExpression(u"SendLaterItem \\d+"_s);
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
    const QString to = u"kde.org"_s;
    info.setTo(to);
    info.setItemId(Akonadi::Item::Id(42));
    info.setSubject(u"Subject"_s);
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

#include "moc_sendlaterconfigtest.cpp"
