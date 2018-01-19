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
#include "kmsystemtray.h"
#include "settings/kmailsettings.h"
#include "kmail_debug.h"
#include <MailCommon/MailKernel>
#include <MailCommon/MailUtil>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusServiceWatcher>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusConnectionInterface>
#include <QApplication>
#include <QTimer>

#include <AkonadiCore/ChangeRecorder>
#include <AkonadiCore/EntityTreeModel>
#include <AkonadiCore/EntityMimeTypeFilterModel>
#include <AkonadiCore/CollectionStatistics>
#include <AkonadiCore/NewMailNotifierAttribute>

using namespace KMail;

UnityServiceManager::UnityServiceManager(QObject *parent)
    : QObject(parent)
    , mUnityServiceWatcher(new QDBusServiceWatcher(this))
{
    connect(kmkernel->folderCollectionMonitor(), &Akonadi::Monitor::collectionStatisticsChanged, this, &UnityServiceManager::slotCollectionStatisticsChanged);

    connect(kmkernel->folderCollectionMonitor(), &Akonadi::Monitor::collectionAdded, this, &UnityServiceManager::initListOfCollection);
    connect(kmkernel->folderCollectionMonitor(), &Akonadi::Monitor::collectionRemoved, this, &UnityServiceManager::initListOfCollection);
    connect(kmkernel->folderCollectionMonitor(), &Akonadi::Monitor::collectionSubscribed, this, &UnityServiceManager::initListOfCollection);
    connect(kmkernel->folderCollectionMonitor(), &Akonadi::Monitor::collectionUnsubscribed, this, &UnityServiceManager::initListOfCollection);
    initListOfCollection();
    initUnity();
}

UnityServiceManager::~UnityServiceManager()
{
    mSystemTray = nullptr;
}

bool UnityServiceManager::excludeFolder(const Akonadi::Collection &collection) const
{
    if (!collection.isValid() || !collection.contentMimeTypes().contains(KMime::Message::mimeType())) {
        return true;
    }
    if (CommonKernel->outboxCollectionFolder() == collection
        || CommonKernel->sentCollectionFolder() == collection
        || CommonKernel->templatesCollectionFolder() == collection
        || CommonKernel->trashCollectionFolder() == collection
        || CommonKernel->draftsCollectionFolder() == collection) {
        return true;
    }

    if (MailCommon::Util::isVirtualCollection(collection)) {
        return true;
    }
    return false;
}

void UnityServiceManager::unreadMail(const QAbstractItemModel *model, const QModelIndex &parentIndex)
{
    const int rowCount = model->rowCount(parentIndex);
    for (int row = 0; row < rowCount; ++row) {
        const QModelIndex index = model->index(row, 0, parentIndex);
        const Akonadi::Collection collection = model->data(index, Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();

        if (!excludeFolder(collection)) {
            const Akonadi::CollectionStatistics statistics = collection.statistics();
            const qint64 count = qMax(0LL, statistics.unreadCount());

            if (count > 0) {
                if (!ignoreNewMailInFolder(collection)) {
                    mCount += count;
                }
            }
        }
        if (model->rowCount(index) > 0) {
            unreadMail(model, index);
        }
    }
    if (mSystemTray) {
        // Update tooltip to reflect count of unread messages
        mSystemTray->updateToolTip(mCount);
    }
}

void UnityServiceManager::updateSystemTray()
{
    initListOfCollection();
}

void UnityServiceManager::initListOfCollection()
{
    mCount = 0;
    const QAbstractItemModel *model = kmkernel->collectionModel();
    if (model->rowCount() == 0) {
        QTimer::singleShot(1000, this, &UnityServiceManager::initListOfCollection);
        return;
    }
    unreadMail(model);
    if (mSystemTray) {
        mSystemTray->updateStatus(mCount);
    }

    //qCDebug(KMAIL_LOG)<<" mCount :"<<mCount;
    updateCount();
}

void UnityServiceManager::slotCollectionStatisticsChanged(Akonadi::Collection::Id id, const Akonadi::CollectionStatistics &)
{
    //Exclude sent mail folder

    if (CommonKernel->outboxCollectionFolder().id() == id
        || CommonKernel->sentCollectionFolder().id() == id
        || CommonKernel->templatesCollectionFolder().id() == id
        || CommonKernel->trashCollectionFolder().id() == id
        || CommonKernel->draftsCollectionFolder().id() == id) {
        return;
    }
    initListOfCollection();
}

void UnityServiceManager::updateCount()
{
    if (mSystemTray) {
        mSystemTray->updateCount(mCount);
    }
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

bool UnityServiceManager::ignoreNewMailInFolder(const Akonadi::Collection &collection)
{
    if (collection.hasAttribute<Akonadi::NewMailNotifierAttribute>()) {
        if (collection.attribute<Akonadi::NewMailNotifierAttribute>()->ignoreNewMail()) {
            return true;
        }
    }
    return false;
}

bool UnityServiceManager::haveSystemTrayApplet() const
{
    return mSystemTray != nullptr;
}

bool UnityServiceManager::hasUnreadMail() const
{
    return mCount != 0;
}

bool UnityServiceManager::canQueryClose()
{
    if (!mSystemTray) {
        return true;
    }
    if (hasUnreadMail()) {
        mSystemTray->setStatus(KStatusNotifierItem::Active);
    }
    mSystemTray->hideKMail();
    return false;
}

void UnityServiceManager::toggleSystemTray(QWidget *widget)
{
    if (widget) {
        if (!mSystemTray && KMailSettings::self()->systemTrayEnabled()) {
            mSystemTray = new KMail::KMSystemTray(widget);
            mSystemTray->setUnityServiceManager(this);
            mSystemTray->initialize(mCount);
        } else if (mSystemTray && !KMailSettings::self()->systemTrayEnabled()) {
            // Get rid of system tray on user's request
            qCDebug(KMAIL_LOG) << "deleting systray";
            delete mSystemTray;
            mSystemTray = nullptr;
        }
    }
}
