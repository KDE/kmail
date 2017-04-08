/*
   Copyright (C) 2014-2017 Montel Laurent <montel@kde.org>

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

#include "followupremindershowmessagejob.h"
#include "followupreminderagent_debug.h"
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusConnectionInterface>
#include <ktoolinvocation.h>

FollowUpReminderShowMessageJob::FollowUpReminderShowMessageJob(Akonadi::Item::Id id, QObject *parent)
    : QObject(parent),
      mId(id)
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
