/*
   Copyright (C) 2014-2020 Laurent Montel <montel@kde.org>

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

QTEST_MAIN(SendLaterUtilTest)
