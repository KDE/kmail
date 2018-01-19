/*
   Copyright (C) 2018 Montel Laurent <montel@kde.org>

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

#include "unityservicemanager.h"
#include "kmkernel.h"
#include <MailCommon/MailKernel>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusServiceWatcher>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusConnectionInterface>
#include <QApplication>

#include <AkonadiCore/ChangeRecorder>
#include <AkonadiCore/EntityTreeModel>
#include <AkonadiCore/EntityMimeTypeFilterModel>

using namespace KMail;

UnityServiceManager::UnityServiceManager(QObject *parent)
    : QObject(parent)
    , mUnityServiceWatcher(new QDBusServiceWatcher(this))
    , mCount(0)
{
#if 0
    connect(kmkernel->folderCollectionMonitor(), &Akonadi::Monitor::collectionStatisticsChanged, this, &UnityServiceManager::slotCollectionStatisticsChanged);

    connect(kmkernel->folderCollectionMonitor(), &Akonadi::Monitor::collectionAdded, this, &UnityServiceManager::initListOfCollection);
    connect(kmkernel->folderCollectionMonitor(), &Akonadi::Monitor::collectionRemoved, this, &UnityServiceManager::initListOfCollection);
    connect(kmkernel->folderCollectionMonitor(), &Akonadi::Monitor::collectionSubscribed, this, &UnityServiceManager::initListOfCollection);
    connect(kmkernel->folderCollectionMonitor(), &Akonadi::Monitor::collectionUnsubscribed, this, &UnityServiceManager::initListOfCollection);
#endif
    initUnity();
}

UnityServiceManager::~UnityServiceManager()
{

}

void UnityServiceManager::slotSetUnread(int unread)
{
    mCount = unread;
    updateCount();
}

void UnityServiceManager::updateCount()
{
    if (mUnityServiceAvailable) {
        const QString launcherId = qApp->desktopFileName() + QLatin1String(".desktop");

        const QVariantMap properties{
            {QStringLiteral("count-visible"), mCount > 0},
            {QStringLiteral("count"), mCount}
        };

        QDBusMessage message = QDBusMessage::createSignal(QStringLiteral("/org/kmail2/UnityLauncher"),
                                                          QStringLiteral("com.canonical.Unity.LauncherEntry"),
                                                          QStringLiteral("Update"));
        message.setArguments({launcherId, properties});
        QDBusConnection::sessionBus().send(message);
    }
}

void UnityServiceManager::initUnity()
{
    mUnityServiceWatcher->setConnection(QDBusConnection::sessionBus());
    mUnityServiceWatcher->setWatchMode(QDBusServiceWatcher::WatchForUnregistration | QDBusServiceWatcher::WatchForRegistration);
    mUnityServiceWatcher->addWatchedService(QStringLiteral("com.canonical.Unity"));
    connect(mUnityServiceWatcher, &QDBusServiceWatcher::serviceRegistered, this, [this](const QString &service) {
        Q_UNUSED(service);
        mUnityServiceAvailable = true;
        updateCount();
    });

    connect(mUnityServiceWatcher, &QDBusServiceWatcher::serviceUnregistered, this, [this](const QString &service) {
        Q_UNUSED(service);
        mUnityServiceAvailable = false;
    });

    // QDBusConnectionInterface::isServiceRegistered blocks
    QDBusPendingCall listNamesCall = QDBusConnection::sessionBus().interface()->asyncCall(QStringLiteral("ListNames"));
    QDBusPendingCallWatcher *callWatcher = new QDBusPendingCallWatcher(listNamesCall, this);
    connect(callWatcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
        QDBusPendingReply<QStringList> reply = *watcher;
        watcher->deleteLater();

        if (reply.isError()) {
            return;
        }

        const QStringList &services = reply.value();

        mUnityServiceAvailable = services.contains(QLatin1String("com.canonical.Unity"));
        if (mUnityServiceAvailable) {
            updateCount();
        }
    });
}
