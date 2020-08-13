/*
   SPDX-FileCopyrightText: 2014-2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "followupremindershowmessagejob.h"
#include "followupreminderagent_debug.h"
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusConnectionInterface>
#include <KToolInvocation>

FollowUpReminderShowMessageJob::FollowUpReminderShowMessageJob(Akonadi::Item::Id id, QObject *parent)
    : QObject(parent)
    , mId(id)
{
}

FollowUpReminderShowMessageJob::~FollowUpReminderShowMessageJob()
{
}

void FollowUpReminderShowMessageJob::start()
{
    if (mId < 0) {
        qCDebug(FOLLOWUPREMINDERAGENT_LOG) << " value < 0";
        deleteLater();
        return;
    }
    const QString kmailInterface = QStringLiteral("org.kde.kmail");
    if (!QDBusConnection::sessionBus().interface()->isServiceRegistered(kmailInterface)) {
        // Program is not already running, so start it
        QString errmsg;
        if (KToolInvocation::startServiceByDesktopName(QStringLiteral("org.kde.kmail2"), QString(), &errmsg)) {
            qCDebug(FOLLOWUPREMINDERAGENT_LOG) << " Can not start kmail" << errmsg;
            deleteLater();
            return;
        }
    }
    QDBusInterface kmail(kmailInterface, QStringLiteral("/KMail"), QStringLiteral("org.kde.kmail.kmail"));
    kmail.call(QStringLiteral("showMail"), mId);
    deleteLater();
}
