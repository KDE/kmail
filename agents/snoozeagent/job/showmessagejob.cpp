/*
   SPDX-FileCopyrightText: 2014-2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "showmessagejob.h"
#include "snoozeagent_debug.h"
#include <KLocalizedString>

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>

ShowMessageJob::ShowMessageJob(Akonadi::Item::Id id, QObject *parent)
    : KJob(parent)
    , mId(id)
{
}

ShowMessageJob::~ShowMessageJob() = default;

void ShowMessageJob::start()
{
    if (mId < 0) {
        emitResult();
        return;
    }

    const QString kmailInterface = QStringLiteral("org.kde.kmail");
    auto startReply = QDBusConnection::sessionBus().interface()->startService(kmailInterface);
    if (!startReply.isValid()) {
        qCDebug(SNOOZEAGENT_LOG) << "Can not start kmail";
        setError(UserDefinedError);
        setErrorText(i18n("Unable to start KMail application."));
        emitResult();
        return;
    }

    QDBusInterface kmail(kmailInterface, QStringLiteral("/KMail"), QStringLiteral("org.kde.kmail.kmail"));
    if (kmail.isValid()) {
        kmail.call(QStringLiteral("showMail"), mId);
    } else {
        qCWarning(SNOOZEAGENT_LOG) << "Impossible to access the DBus interface";
    }

    emitResult();
}

#include "moc_showmessagejob.cpp"
