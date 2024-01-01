/*
   SPDX-FileCopyrightText: 2012-2024 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "archivemailagent.h"
#include "archivemailagentadaptor.h"
#include "archivemailagentsettings.h"
#include "archivemailmanager.h"
#include <Akonadi/ServerManager>

#include <Akonadi/CollectionFetchScope>
#include <Akonadi/Monitor>
#include <Akonadi/Session>
#include <KMime/Message>
#include <MailCommon/MailKernel>
#include <QDBusConnection>
#include <QTimer>
#include <chrono>
using namespace std::chrono_literals;
// #define DEBUG_ARCHIVEMAILAGENT 1

ArchiveMailAgent::ArchiveMailAgent(const QString &id)
    : Akonadi::AgentBase(id)
    , mTimer(new QTimer(this))
    , mArchiveManager(new ArchiveMailManager(this))
{
    connect(this, &Akonadi::AgentBase::reloadConfiguration, this, &ArchiveMailAgent::reload);

    connect(mArchiveManager, &ArchiveMailManager::needUpdateConfigDialogBox, this, &ArchiveMailAgent::needUpdateConfigDialogBox);

    auto collectionMonitor = new Akonadi::Monitor(this);
    collectionMonitor->setObjectName(QLatin1StringView("ArchiveMailCollectionMonitor"));
    collectionMonitor->fetchCollection(true);
    collectionMonitor->ignoreSession(Akonadi::Session::defaultSession());
    collectionMonitor->collectionFetchScope().setAncestorRetrieval(Akonadi::CollectionFetchScope::All);
    collectionMonitor->setMimeTypeMonitored(KMime::Message::mimeType());

    new ArchiveMailAgentAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/ArchiveMailAgent"), this, QDBusConnection::ExportAdaptors);

    const QString service = Akonadi::ServerManager::self()->agentServiceName(Akonadi::ServerManager::Agent, identifier());

    QDBusConnection::sessionBus().registerService(service);
    connect(collectionMonitor, &Akonadi::Monitor::collectionRemoved, this, &ArchiveMailAgent::mailCollectionRemoved);

    if (enabledAgent()) {
#ifdef DEBUG_ARCHIVEMAILAGENT
        QTimer::singleShot(1s, mArchiveManager, &ArchiveMailManager::load);
#else
        QTimer::singleShot(5min, mArchiveManager, &ArchiveMailManager::load);
#endif
    }

    connect(mTimer, &QTimer::timeout, this, &ArchiveMailAgent::reload);
    // Now we have range support we need to reload each hour.
    mTimer->start(1h);
}

ArchiveMailAgent::~ArchiveMailAgent() = default;

void ArchiveMailAgent::setEnableAgent(bool enabled)
{
    if (enabled != ArchiveMailAgentSettings::enabled()) {
        ArchiveMailAgentSettings::setEnabled(enabled);
        ArchiveMailAgentSettings::self()->save();
        if (!enabled) {
            mTimer->stop();
            pause();
        } else {
            mTimer->start();
        }
    }
}

bool ArchiveMailAgent::enabledAgent() const
{
    return ArchiveMailAgentSettings::enabled();
}

void ArchiveMailAgent::mailCollectionRemoved(const Akonadi::Collection &collection)
{
    mArchiveManager->removeCollection(collection);
}

void ArchiveMailAgent::doSetOnline(bool online)
{
    if (online) {
        resume();
    } else {
        pause();
    }
}

void ArchiveMailAgent::reload()
{
    if (isOnline() && enabledAgent()) {
        mArchiveManager->load();
        mTimer->start();
    }
}

void ArchiveMailAgent::pause()
{
    if (isOnline() && enabledAgent()) {
        mArchiveManager->pause();
    }
}

void ArchiveMailAgent::resume()
{
    if (isOnline() && enabledAgent()) {
        mArchiveManager->resume();
    }
}

QString ArchiveMailAgent::printArchiveListInfo() const
{
    return mArchiveManager->printArchiveListInfo();
}

QString ArchiveMailAgent::printCurrentListInfo() const
{
    return mArchiveManager->printCurrentListInfo();
}

void ArchiveMailAgent::archiveFolder(const QString &path, Akonadi::Collection::Id collectionId)
{
    mArchiveManager->archiveFolder(path, collectionId);
}

AKONADI_AGENT_MAIN(ArchiveMailAgent)

#include "moc_archivemailagent.cpp"
