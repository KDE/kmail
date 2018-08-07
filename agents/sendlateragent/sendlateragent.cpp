/*
   Copyright (C) 2013-2018 Montel Laurent <montel@kde.org>

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

#include "sendlateragent.h"
#include "sendlatermanager.h"
#include "sendlaterconfiguredialog.h"
#include "sendlaterinfo.h"
#include "sendlaterutil.h"
#include "sendlateragentadaptor.h"
#include "sendlateragentsettings.h"
#include "sendlaterremovemessagejob.h"
#include "sendlateragent_debug.h"
#include <AkonadiCore/ServerManager>
#include <Akonadi/KMime/SpecialMailCollections>
#include <AgentInstance>
#include <AgentManager>
#include <kdbusconnectionpool.h>
#include <changerecorder.h>
#include <itemfetchscope.h>
#include <AkonadiCore/session.h>
#include <AttributeFactory>
#include <CollectionFetchScope>
#include <KMime/Message>

#include <KWindowSystem>
#include <Kdelibs4ConfigMigrator>

#include <QPointer>

//#define DEBUG_SENDLATERAGENT 1

SendLaterAgent::SendLaterAgent(const QString &id)
    : Akonadi::AgentBase(id)
    , mAgentInitialized(false)
{
    Kdelibs4ConfigMigrator migrate(QStringLiteral("sendlateragent"));
    migrate.setConfigFiles(QStringList() << QStringLiteral("akonadi_sendlater_agentrc") << QStringLiteral("akonadi_sendlater_agent.notifyrc"));
    migrate.migrate();

    mManager = new SendLaterManager(this);
    connect(mManager, &SendLaterManager::needUpdateConfigDialogBox, this, &SendLaterAgent::needUpdateConfigDialogBox);
    new SendLaterAgentAdaptor(this);
    KDBusConnectionPool::threadConnection().registerObject(QStringLiteral("/SendLaterAgent"), this, QDBusConnection::ExportAdaptors);

    const QString service = Akonadi::ServerManager::self()->agentServiceName(Akonadi::ServerManager::Agent, QStringLiteral("akonadi_sendlater_agent"));

    KDBusConnectionPool::threadConnection().registerService(service);

    changeRecorder()->setMimeTypeMonitored(KMime::Message::mimeType());
    changeRecorder()->itemFetchScope().setCacheOnly(true);
    changeRecorder()->itemFetchScope().setFetchModificationTime(false);
    changeRecorder()->setChangeRecordingEnabled(false);
    changeRecorder()->ignoreSession(Akonadi::Session::defaultSession());
    setNeedsNetwork(true);

    if (SendLaterAgentSettings::enabled()) {
#ifdef DEBUG_SENDLATERAGENT
        QTimer::singleShot(1000, this, &SendLaterAgent::slotStartAgent);
#else
        QTimer::singleShot(1000 * 60 * 4, this, &SendLaterAgent::slotStartAgent);
#endif
    }
    // For extra safety, check list every hour, in case we didn't properly get
    // notified about the network going up or down.
    QTimer *reloadListTimer = new QTimer(this);
    connect(reloadListTimer, &QTimer::timeout, this, &SendLaterAgent::reload);
    reloadListTimer->start(1000 * 60 * 60); //1 hour
}

SendLaterAgent::~SendLaterAgent()
{
}

void SendLaterAgent::slotStartAgent()
{
    mAgentInitialized = true;
    if (isOnline()) {
        mManager->load();
    }
}

void SendLaterAgent::doSetOnline(bool online)
{
    if (mAgentInitialized) {
        if (online) {
            reload();
        } else {
            mManager->stopAll();
        }
    }
}

void SendLaterAgent::reload()
{
    qCDebug(SENDLATERAGENT_LOG) << " void SendLaterAgent::reload()";
    if (SendLaterAgentSettings::enabled()) {
        mManager->load(true);
    }
}

void SendLaterAgent::setEnableAgent(bool enabled)
{
    if (SendLaterAgentSettings::enabled() == enabled) {
        return;
    }

    SendLaterAgentSettings::setEnabled(enabled);
    SendLaterAgentSettings::self()->save();
    if (enabled) {
        mManager->load();
    } else {
        mManager->stopAll();
    }
}

bool SendLaterAgent::enabledAgent() const
{
    return SendLaterAgentSettings::enabled();
}

void SendLaterAgent::configure(WId windowId)
{
    QPointer<SendLaterConfigureDialog> dialog = new SendLaterConfigureDialog();
    if (windowId) {
        KWindowSystem::setMainWindow(dialog, windowId);
    }
    connect(this, &SendLaterAgent::needUpdateConfigDialogBox, dialog.data(), &SendLaterConfigureDialog::slotNeedToReloadConfig);
    connect(dialog.data(), &SendLaterConfigureDialog::sendNow, this, &SendLaterAgent::slotSendNow);
    if (dialog->exec()) {
        mManager->load();
        QList<Akonadi::Item::Id> listMessage = dialog->messagesToRemove();
        if (!listMessage.isEmpty()) {
            //Will delete in specific job when done.
            SendLaterRemoveMessageJob *sendlaterremovejob = new SendLaterRemoveMessageJob(listMessage, this);
            sendlaterremovejob->start();
        }
    }
    delete dialog;
}

void SendLaterAgent::slotSendNow(Akonadi::Item::Id id)
{
    mManager->sendNow(id);
}

void SendLaterAgent::itemsRemoved(const Akonadi::Item::List &items)
{
    for (const Akonadi::Item &item : items) {
        mManager->itemRemoved(item.id());
    }
}

void SendLaterAgent::itemsMoved(const Akonadi::Item::List &items, const Akonadi::Collection & /*sourceCollection*/, const Akonadi::Collection &destinationCollection)
{
    if (Akonadi::SpecialMailCollections::self()->specialCollectionType(destinationCollection) != Akonadi::SpecialMailCollections::Trash) {
        return;
    }
    itemsRemoved(items);
}

QString SendLaterAgent::printDebugInfo()
{
    return mManager->printDebugInfo();
}

AKONADI_AGENT_MAIN(SendLaterAgent)
