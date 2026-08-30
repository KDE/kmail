/*
   SPDX-FileCopyrightText: 2014-2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "followupremindershowmessagejob.h"
#include "followupreminderagent_debug.h"
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
using namespace Qt::Literals::StringLiterals;

FollowUpReminderShowMessageJob::FollowUpReminderShowMessageJob(Akonadi::Item::Id id, QObject *parent)
    : QObject(parent)
    , mId(id)
{
}

FollowUpReminderShowMessageJob::~FollowUpReminderShowMessageJob() = default;

void FollowUpReminderShowMessageJob::start()
{
    if (mId < 0) {
        qCWarning(FOLLOWUPREMINDERAGENT_LOG) << " value < 0";
        deleteLater();
        return;
    }
    const QString kmailInterface = u"org.kde.kmail"_s;
    if (QDBusReply<bool> reply = QDBusConnection::sessionBus().interface()->isServiceRegistered(kmailInterface); !reply.isValid() || !reply.value()) {
        // Program is not already running, so start it
        if (!QDBusConnection::sessionBus().interface()->startService(u"org.kde.kmail"_s).isValid()) {
            qCDebug(FOLLOWUPREMINDERAGENT_LOG) << " Can not start kmail";
            deleteLater();
            return;
        }
    }
    if (QDBusInterface kmail(kmailInterface, u"/KMail"_s, u"org.kde.kmail.kmail"_s); kmail.isValid()) {
        kmail.call(u"showMail"_s, mId);
    } else {
        qCWarning(FOLLOWUPREMINDERAGENT_LOG) << "Impossible to access to DBus interface";
    }
    deleteLater();
}

#include "moc_followupremindershowmessagejob.cpp"
