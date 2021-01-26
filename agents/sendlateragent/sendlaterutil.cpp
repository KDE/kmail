/*
   SPDX-FileCopyrightText: 2013-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "sendlaterutil.h"
#include "sendlateragent_debug.h"
#include "sendlateragentsettings.h"

#include <MessageComposer/SendLaterInfo>

#include <AkonadiCore/ServerManager>
#include <KConfigGroup>

#include <QStringList>

bool SendLaterUtil::compareSendLaterInfo(MessageComposer::SendLaterInfo *left, MessageComposer::SendLaterInfo *right)
{
    if (left->dateTime() == right->dateTime()) {
        // Set no recursive first.
        if (left->isRecurrence()) {
            return false;
        }
    }
    return left->dateTime() < right->dateTime();
}

static QDateTime updateRecurence(MessageComposer::SendLaterInfo *info, QDateTime dateTime)
{
    switch (info->recurrenceUnit()) {
    case MessageComposer::SendLaterInfo::Days:
        dateTime = dateTime.addDays(info->recurrenceEachValue());
        break;
    case MessageComposer::SendLaterInfo::Weeks:
        dateTime = dateTime.addDays(info->recurrenceEachValue() * 7);
        break;
    case MessageComposer::SendLaterInfo::Months:
        dateTime = dateTime.addMonths(info->recurrenceEachValue());
        break;
    case MessageComposer::SendLaterInfo::Years:
        dateTime = dateTime.addYears(info->recurrenceEachValue());
        break;
    }
    return dateTime;
}

void SendLaterUtil::changeRecurrentDate(MessageComposer::SendLaterInfo *info)
{
    if (info && info->isRecurrence()) {
        qCDebug(SENDLATERAGENT_LOG) << "BEFORE SendLaterUtil::changeRecurrentDate " << info->dateTime().toString();
        QDateTime newInfoDateTime = info->dateTime();
        newInfoDateTime = updateRecurence(info, newInfoDateTime);
        qCDebug(SENDLATERAGENT_LOG) << " QDateTime::currentDateTime()" << QDateTime::currentDateTime().toString();
        while (newInfoDateTime <= QDateTime::currentDateTime()) {
            newInfoDateTime = updateRecurence(info, newInfoDateTime);
        }
        info->setDateTime(newInfoDateTime);
        qCDebug(SENDLATERAGENT_LOG) << "AFTER SendLaterUtil::changeRecurrentDate " << info->dateTime().toString() << " info" << info << "New date"
                                    << newInfoDateTime;
        writeSendLaterInfo(defaultConfig(), info);
    }
}

KSharedConfig::Ptr SendLaterUtil::defaultConfig()
{
    return KSharedConfig::openConfig(QStringLiteral("akonadi_sendlater_agentrc"), KConfig::SimpleConfig);
}

MessageComposer::SendLaterInfo *SendLaterUtil::readSendLaterInfo(KConfigGroup &config)
{
    auto info = new MessageComposer::SendLaterInfo;
    if (config.hasKey(QStringLiteral("lastDateTimeSend"))) {
        info->setLastDateTimeSend(QDateTime::fromString(config.readEntry("lastDateTimeSend"), Qt::ISODate));
    }
    info->setDateTime(config.readEntry("date", QDateTime::currentDateTime()));
    info->setRecurrence(config.readEntry("recurrence", false));
    info->setRecurrenceEachValue(config.readEntry("recurrenceValue", 1));
    info->setRecurrenceUnit(static_cast<MessageComposer::SendLaterInfo::RecurrenceUnit>(
        config.readEntry("recurrenceUnit", static_cast<int>(MessageComposer::SendLaterInfo::Days))));
    info->setItemId(config.readEntry("itemId", -1));
    info->setSubject(config.readEntry("subject"));
    info->setTo(config.readEntry("to"));

    return info;
}

void SendLaterUtil::writeSendLaterInfo(KSharedConfig::Ptr config, MessageComposer::SendLaterInfo *info)
{
    if (!info || !info->isValid()) {
        return;
    }

    const QString groupName = SendLaterUtil::sendLaterPattern().arg(info->itemId());
    // first, delete all filter groups:
    const QStringList filterGroups = config->groupList();
    for (const QString &group : filterGroups) {
        if (group == groupName) {
            config->deleteGroup(group);
        }
    }
    KConfigGroup group = config->group(groupName);
    if (info->lastDateTimeSend().isValid()) {
        group.writeEntry("lastDateTimeSend", info->lastDateTimeSend().toString(Qt::ISODate));
    }
    group.writeEntry("date", info->dateTime());
    group.writeEntry("recurrence", info->isRecurrence());
    group.writeEntry("recurrenceValue", info->recurrenceEachValue());
    group.writeEntry("recurrenceUnit", static_cast<int>(info->recurrenceUnit()));
    group.writeEntry("itemId", info->itemId());
    group.writeEntry("subject", info->subject());
    group.writeEntry("to", info->to());
    config->sync();
    config->reparseConfiguration();
    qCDebug(SENDLATERAGENT_LOG) << " reparse config";
}

void SendLaterUtil::forceReparseConfiguration()
{
    SendLaterAgentSettings::self()->save();
    SendLaterAgentSettings::self()->config()->reparseConfiguration();
}

bool SendLaterUtil::sentLaterAgentEnabled()
{
    return SendLaterAgentSettings::self()->enabled();
}

QString SendLaterUtil::sendLaterPattern()
{
    return QStringLiteral("SendLaterItem %1");
}
