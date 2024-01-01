/*
   SPDX-FileCopyrightText: 2020-2024 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "accountinfosource.h"
#include <Akonadi/AgentInstance>
#include <KLocalizedString>
#include <KUserFeedback/Provider>
#include <MailCommon/MailUtil>
#include <MailTransport/TransportManager>
#include <QVariant>

AccountInfoSource::AccountInfoSource()
    : KUserFeedback::AbstractDataSource(QStringLiteral("accounts"), KUserFeedback::Provider::DetailedSystemInformation)
{
}

QString AccountInfoSource::name() const
{
    return i18n("Account information");
}

QString AccountInfoSource::description() const
{
    return i18n("Number and type of accounts configured in KMail (receiver and sender).");
}

void AccountInfoSource::updateAccountInfo(const QString &resourceName, int numberOfResource, QVariantList &l)
{
    QVariantMap m;
    m.insert(QStringLiteral("name"), resourceName);
    m.insert(QStringLiteral("number"), numberOfResource);
    l.push_back(m);
}

QVariant AccountInfoSource::data()
{
    const Akonadi::AgentInstance::List lst = MailCommon::Util::agentInstances();
    int numberOfImap = 0;
    int numberOfPop3 = 0;
    int numberOfKolab = 0;
    int numberOfEws = 0;
    int numberOfMaildir = 0;
    int numberOfMbox = 0;
    for (const Akonadi::AgentInstance &type : lst) {
        const QString identifier = type.identifier();
        if (identifier.startsWith(QLatin1String("akonadi_pop3_resource"))) {
            numberOfPop3++;
        } else if (identifier.startsWith(QLatin1String("akonadi_imap_resource"))) {
            numberOfImap++;
        } else if (identifier.startsWith(QLatin1String("akonadi_kolab_resource"))) {
            numberOfKolab++;
        } else if (identifier.startsWith(QLatin1String("akonadi_ews_resource"))) {
            numberOfEws++;
        } else if (identifier.startsWith(QLatin1String("akonadi_maildir_resource"))) {
            numberOfMaildir++;
        } else if (identifier.startsWith(QLatin1String("akonadi_mbox_resource"))) {
            numberOfMbox++;
        }
    }
    QVariantList l;
    if (numberOfImap > 0) {
        updateAccountInfo(QStringLiteral("imap"), numberOfImap, l);
    }
    if (numberOfPop3 > 0) {
        updateAccountInfo(QStringLiteral("pop3"), numberOfPop3, l);
    }
    if (numberOfKolab > 0) {
        updateAccountInfo(QStringLiteral("kolab"), numberOfKolab, l);
    }
    if (numberOfEws > 0) {
        updateAccountInfo(QStringLiteral("ews"), numberOfEws, l);
    }
    if (numberOfMaildir > 0) {
        updateAccountInfo(QStringLiteral("maildir"), numberOfMaildir, l);
    }
    if (numberOfMbox > 0) {
        updateAccountInfo(QStringLiteral("mbox"), numberOfMbox, l);
    }

    // Mail Transport
    QVariantMap m;
    m.insert(QStringLiteral("name"), QStringLiteral("sender"));
    m.insert(QStringLiteral("number"), MailTransport::TransportManager::self()->transports().count());
    l.push_back(m);

    return l;
}
