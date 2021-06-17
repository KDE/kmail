/*
   SPDX-FileCopyrightText: 2020-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "accountinfosource.h"
#include <AkonadiCore/AgentInstance>
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
        // TODO add more
    }
    QVariantList l;
    if (numberOfImap > 0) {
        QVariantMap m;
        m.insert(QStringLiteral("name"), QStringLiteral("imap"));
        m.insert(QStringLiteral("number"), numberOfImap);
        l.push_back(m);
    }
    if (numberOfPop3 > 0) {
        QVariantMap m;
        m.insert(QStringLiteral("name"), QStringLiteral("pop3"));
        m.insert(QStringLiteral("number"), numberOfPop3);
        l.push_back(m);
    }
    if (numberOfKolab > 0) {
        QVariantMap m;
        m.insert(QStringLiteral("name"), QStringLiteral("kolab"));
        m.insert(QStringLiteral("number"), numberOfKolab);
        l.push_back(m);
    }
    if (numberOfEws > 0) {
        QVariantMap m;
        m.insert(QStringLiteral("name"), QStringLiteral("ews"));
        m.insert(QStringLiteral("number"), numberOfEws);
        l.push_back(m);
    }
    if (numberOfMaildir > 0) {
        QVariantMap m;
        m.insert(QStringLiteral("name"), QStringLiteral("maildir"));
        m.insert(QStringLiteral("number"), numberOfMaildir);
        l.push_back(m);
    }
    if (numberOfMbox > 0) {
        QVariantMap m;
        m.insert(QStringLiteral("name"), QStringLiteral("mbox"));
        m.insert(QStringLiteral("number"), numberOfMbox);
        l.push_back(m);
    }

    // Mail Transport
    QVariantMap m;
    m.insert(QStringLiteral("name"), QStringLiteral("sender"));
    m.insert(QStringLiteral("number"), MailTransport::TransportManager::self()->transports().count());
    l.push_back(m);

    return l;
}
