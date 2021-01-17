/*
   SPDX-FileCopyrightText: 2014-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "sendlaterutiltest.h"
#include "sendlaterutil.h"

#include <MessageComposer/SendLaterInfo>

#include <QTest>
#include <QDateTime>
#include <KConfigGroup>
#include <KSharedConfig>
#include <QStandardPaths>
#include <sendlaterinfo.h>

SendLaterUtilTest::SendLaterUtilTest(QObject *parent)
    : QObject(parent)
{
    QStandardPaths::setTestModeEnabled(true);
}

void SendLaterUtilTest::shouldRestoreFromSettings()
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
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    info.setDateTime(QDateTime(date));
    info.setLastDateTimeSend(QDateTime(date));
#else
    info.setDateTime(QDateTime(date.startOfDay()));
    info.setLastDateTimeSend(QDateTime(date.startOfDay()));
#endif
    SendLaterUtil::writeSendLaterInfo(KSharedConfig::openConfig(), &info);

    KConfigGroup grp(KSharedConfig::openConfig(), SendLaterUtil::sendLaterPattern().arg(42));
    std::unique_ptr<MessageComposer::SendLaterInfo> restoreInfo{SendLaterUtil::readSendLaterInfo(grp)};
    QCOMPARE(info, *restoreInfo);
}

QTEST_GUILESS_MAIN(SendLaterUtilTest)
