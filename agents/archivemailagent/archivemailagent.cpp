/*
   Copyright (C) 2012-2016 Montel Laurent <montel@kde.org>

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

#include "archivemailagent.h"
#include "archivemailagentadaptor.h"
#include "archivemaildialog.h"
#include "archivemailmanager.h"
#include "archivemailagentsettings.h"

#include <KLocalizedString>
#include <MailCommon/MailKernel>
#include <kdbusconnectionpool.h>
#include <Monitor>
#include <Session>
#include <CollectionFetchScope>
#include <KMime/Message>
#include <KWindowSystem>
#include <QTimer>
#include <QPointer>

#include <kdelibs4configmigrator.h>

//#define DEBUG_ARCHIVEMAILAGENT 1

ArchiveMailAgent::ArchiveMailAgent(const QString &id)
    : Akonadi::AgentBase(id)
{
    Kdelibs4ConfigMigrator migrate(QStringLiteral("archivemailagent"));
    migrate.setConfigFiles(QStringList() << QStringLiteral("akonadi_archivemail_agentrc") << QStringLiteral("akonadi_archivemail_agent.notifyrc"));
    migrate.migrate();

    mArchiveManager = new ArchiveMailManager(this);
    connect(mArchiveManager, &ArchiveMailManager::needUpdateConfigDialogBox, this, &ArchiveMailAgent::needUpdateConfigDialogBox);

    Akonadi::Monitor *collectionMonitor = new Akonadi::Monitor(this);
    collectionMonitor->fetchCollection(true);
    collectionMonitor->ignoreSession(Akonadi::Session::defaultSession());
    collectionMonitor->collectionFetchScope().setAncestorRetrieval(Akonadi::CollectionFetchScope::All);
    collectionMonitor->setMimeTypeMonitored(KMime::Message::mimeType());

    new ArchiveMailAgentAdaptor(this);
    KDBusConnectionPool::threadConnection().registerObject(QStringLiteral("/ArchiveMailAgent"), this, QDBusConnection::ExportAdaptors);
    KDBusConnectionPool::threadConnection().registerService(QStringLiteral("org.freedesktop.Akonadi.ArchiveMailAgent"));
    connect(collectionMonitor, &Akonadi::Monitor::collectionRemoved, this, &ArchiveMailAgent::mailCollectionRemoved);

    if (enabledAgent()) {
#ifdef DEBUG_ARCHIVEMAILAGENT
        QTimer::singleShot(1000, mArchiveManager, &ArchiveMailManager::load);
#else
        QTimer::singleShot(1000 * 60 * 5, mArchiveManager, &ArchiveMailManager::load);
#endif
    }

    mTimer = new QTimer(this);
    connect(mTimer, &QTimer::timeout, this, &ArchiveMailAgent::reload);
    mTimer->start(24 * 60 * 60 * 1000);
}

ArchiveMailAgent::~ArchiveMailAgent()
{
}

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

void ArchiveMailAgent::showConfigureDialog(qlonglong windowId)
{
    QPointer<ArchiveMailDialog> dialog = new ArchiveMailDialog();
    if (windowId) {
#ifndef Q_OS_WIN
        KWindowSystem::setMainWindow(dialog, windowId);
#else
        KWindowSystem::setMainWindow(dialog, (HWND)windowId);
#endif
    }
    connect(dialog.data(), &ArchiveMailDialog::archiveNow, mArchiveManager, &ArchiveMailManager::slotArchiveNow);
    connect(this, &ArchiveMailAgent::needUpdateConfigDialogBox, dialog.data(), &ArchiveMailDialog::slotNeedReloadConfig);
    if (dialog->exec()) {
        mArchiveManager->load();
    }
    delete dialog;
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

void ArchiveMailAgent::configure(WId windowId)
{
    showConfigureDialog((qulonglong)windowId);
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

QString ArchiveMailAgent::printArchiveListInfo()
{
    return mArchiveManager->printArchiveListInfo();
}

QString ArchiveMailAgent::printCurrentListInfo()
{
    return mArchiveManager->printCurrentListInfo();
}

void ArchiveMailAgent::archiveFolder(const QString &path, Akonadi::Collection::Id collectionId)
{
    mArchiveManager->archiveFolder(path, collectionId);
}

AKONADI_AGENT_MAIN(ArchiveMailAgent)

