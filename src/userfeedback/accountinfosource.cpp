/*
   Copyright (C) 2020 Laurent Montel <montel@kde.org>

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

#include "accountinfosource.h"
#include <QVariant>
#include <KUserFeedback/Provider>
#include <KLocalizedString>
#include <MailCommon/MailUtil>
#include <AkonadiCore/AgentInstance>
#include <MailTransport/TransportManager>

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
    return i18n("Account configured in KMail (receiver and sender).");
}

QVariant AccountInfoSource::data()
{
    const Akonadi::AgentInstance::List lst = MailCommon::Util::agentInstances();
    int numberOfImap = 0;
    int numberOfPop3 = 0;
    int numberOfKolab = 0;
    int numberOfEws = 0;
    int numberOfMaildir = 0;
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
        }
        //TODO add more
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

    //Mail Transport
    QVariantMap m;
    m.insert(QStringLiteral("name"), QStringLiteral("sender"));
    m.insert(QStringLiteral("number"), MailTransport::TransportManager::self()->transports().count());
    l.push_back(m);

    return l;
}
