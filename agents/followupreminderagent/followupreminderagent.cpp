/*
   Copyright (C) 2014-2018 Montel Laurent <montel@kde.org>

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

#include "followupreminderagent.h"
#include "followupremindermanager.h"
#include "FollowupReminder/FollowUpReminderUtil"
#include "followupreminderadaptor.h"
#include "followupreminderagentsettings.h"
#include <KMime/Message>

#include <AkonadiCore/ChangeRecorder>
#include <AkonadiCore/ItemFetchScope>
#include <kdbusconnectionpool.h>

#include <Kdelibs4ConfigMigrator>
#include <AkonadiCore/Session>
#include <AkonadiCore/CollectionFetchScope>
#include <AkonadiCore/ServerManager>

#include <QPointer>
#include "followupreminderagent_debug.h"
#include <QTimer>

FollowUpReminderAgent::FollowUpReminderAgent(const QString &id)
    : Akonadi::AgentBase(id)
{
    Kdelibs4ConfigMigrator migrate(QStringLiteral("followupreminderagent"));
    migrate.setConfigFiles(QStringList() << QStringLiteral("akonadi_followupreminder_agentrc") << QStringLiteral("akonadi_followupreminder_agent.notifyrc"));
    migrate.migrate();

    new FollowUpReminderAgentAdaptor(this);
    KDBusConnectionPool::threadConnection().registerObject(QStringLiteral("/FollowUpReminder"), this, QDBusConnection::ExportAdaptors);
    const QString service = Akonadi::ServerManager::self()->agentServiceName(Akonadi::ServerManager::Agent, QStringLiteral("akonadi_followupreminder_agent"));
    KDBusConnectionPool::threadConnection().registerService(service);
    mManager = new FollowUpReminderManager(this);
    setNeedsNetwork(true);

    changeRecorder()->setMimeTypeMonitored(KMime::Message::mimeType());
    changeRecorder()->itemFetchScope().setCacheOnly(true);
    changeRecorder()->itemFetchScope().setFetchModificationTime(false);
    changeRecorder()->fetchCollection(true);
    changeRecorder()->setChangeRecordingEnabled(false);
    changeRecorder()->ignoreSession(Akonadi::Session::defaultSession());
    changeRecorder()->collectionFetchScope().setAncestorRetrieval(Akonadi::CollectionFetchScope::All);
    changeRecorder()->setCollectionMonitored(Akonadi::Collection::root(), true);

    if (FollowUpReminderAgentSettings::enabled()) {
        mManager->load();
    }

    mTimer = new QTimer(this);
    connect(mTimer, &QTimer::timeout, this, &FollowUpReminderAgent::reload);
    //Reload all each 24hours
    mTimer->start(24 * 60 * 60 * 1000);
}

FollowUpReminderAgent::~FollowUpReminderAgent()
{
}

void FollowUpReminderAgent::setEnableAgent(bool enabled)
{
    if (FollowUpReminderAgentSettings::self()->enabled() == enabled) {
        return;
    }

    FollowUpReminderAgentSettings::self()->setEnabled(enabled);
    FollowUpReminderAgentSettings::self()->save();
    if (enabled) {
        mManager->load();
        mTimer->start();
    } else {
        mTimer->stop();
    }
}

bool FollowUpReminderAgent::enabledAgent() const
{
    return FollowUpReminderAgentSettings::self()->enabled();
}

void FollowUpReminderAgent::itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection)
{
    if (!enabledAgent()) {
        return;
    }

    if (item.mimeType() != KMime::Message::mimeType()) {
        qCDebug(FOLLOWUPREMINDERAGENT_LOG) << "FollowUpReminderAgent::itemAdded called for a non-message item!";
        return;
    }
    mManager->checkFollowUp(item, collection);
}

void FollowUpReminderAgent::reload()
{
    if (enabledAgent()) {
        mManager->load(true);
        mTimer->start();
    }
}

QString FollowUpReminderAgent::printDebugInfo()
{
    return mManager->printDebugInfo();
}

AKONADI_AGENT_MAIN(FollowUpReminderAgent)
