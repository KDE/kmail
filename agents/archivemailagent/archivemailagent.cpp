/*
   SPDX-FileCopyrightText: 2012-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "archivemailagent.h"
#include "archivemailagentadaptor.h"
#include "archivemailagentsettings.h"
#include "archivemailmanager.h"
#include <AkonadiCore/ServerManager>

#include <CollectionFetchScope>
#include <KMime/Message>
#include <MailCommon/MailKernel>
#include <Monitor>
#include <QDBusConnection>
#include <QTimer>
#include <Session>
#include <chrono>
#include <kcoreaddons_version.h>
using namespace std::chrono_literals;
#if KCOREADDONS_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <Kdelibs4ConfigMigrator>
#endif
//#define DEBUG_ARCHIVEMAILAGENT 1

ArchiveMailAgent::ArchiveMailAgent(const QString &id)
    : Akonadi::AgentBase(id)
    , mArchiveManager(new ArchiveMailManager(this))
{
#if KCOREADDONS_VERSION < QT_VERSION_CHECK(6, 0, 0)
    Kdelibs4ConfigMigrator migrate(QStringLiteral("archivemailagent"));
    migrate.setConfigFiles(QStringList() << QStringLiteral("akonadi_archivemail_agentrc") << QStringLiteral("akonadi_archivemail_agent.notifyrc"));
    migrate.migrate();
#endif

    connect(this, &Akonadi::AgentBase::reloadConfiguration, this, &ArchiveMailAgent::reload);

    connect(mArchiveManager, &ArchiveMailManager::needUpdateConfigDialogBox, this, &ArchiveMailAgent::needUpdateConfigDialogBox);

    auto collectionMonitor = new Akonadi::Monitor(this);
    collectionMonitor->setObjectName(QStringLiteral("ArchiveMailCollectionMonitor"));
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
        QTimer::singleShot(1000, mArchiveManager, &ArchiveMailManager::load);
#else
        QTimer::singleShot(5min, mArchiveManager, &ArchiveMailManager::load);
#endif
    }

    mTimer = new QTimer(this);
    connect(mTimer, &QTimer::timeout, this, &ArchiveMailAgent::reload);
    mTimer->start(24h);
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
