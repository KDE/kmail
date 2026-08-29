/*
   SPDX-FileCopyrightText: 2018-2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "unityservicemanager.h"

#include "kmail_debug.h"
#include "kmkernel.h"
#include "settings/kmailsettings.h"
#include <MailCommon/MailKernel>
#include <MailCommon/MailUtil>

#include <QApplication>
#include <QTimer>

#include <Akonadi/ChangeRecorder>
#include <Akonadi/CollectionStatistics>
#include <Akonadi/EntityMimeTypeFilterModel>
#include <Akonadi/EntityTreeModel>
#include <Akonadi/NewMailNotifierAttribute>
#include <chrono>

using namespace std::chrono_literals;
using namespace Qt::Literals::StringLiterals;

using namespace KMail;

UnityServiceManager::UnityServiceManager(QObject *parent)
    : QObject(parent)
{
    connect(kmkernel->folderCollectionMonitor(), &Akonadi::Monitor::collectionStatisticsChanged, this, &UnityServiceManager::slotCollectionStatisticsChanged);

    connect(kmkernel->folderCollectionMonitor(), &Akonadi::Monitor::collectionAdded, this, &UnityServiceManager::initListOfCollection);
    connect(kmkernel->folderCollectionMonitor(), &Akonadi::Monitor::collectionRemoved, this, &UnityServiceManager::initListOfCollection);
    connect(kmkernel->folderCollectionMonitor(), &Akonadi::Monitor::collectionSubscribed, this, &UnityServiceManager::initListOfCollection);
    connect(kmkernel->folderCollectionMonitor(), &Akonadi::Monitor::collectionUnsubscribed, this, &UnityServiceManager::initListOfCollection);
    initListOfCollection();
}

UnityServiceManager::~UnityServiceManager()
{
    delete mSystemTray;
}

bool UnityServiceManager::excludeFolder(const Akonadi::Collection &collection) const
{
    if (!collection.isValid() || !collection.contentMimeTypes().contains(KMime::Message::mimeType())) {
        return true;
    }
    if (CommonKernel->outboxCollectionFolder() == collection || CommonKernel->sentCollectionFolder() == collection
        || CommonKernel->templatesCollectionFolder() == collection || CommonKernel->trashCollectionFolder() == collection
        || CommonKernel->draftsCollectionFolder() == collection || CommonKernel->spamsCollectionFolder() == collection) {
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
        if (const auto collection = model->data(index, Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>(); !excludeFolder(collection)) {
            const Akonadi::CollectionStatistics statistics = collection.statistics();
            if (const qint64 count = qMax(0LL, statistics.unreadCount()); count > 0) {
                if (!ignoreNewMailInFolder(collection)) {
                    mCount += count;
                }
            }
        }
        if (model->hasChildren(index)) {
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
        QTimer::singleShot(1s, this, &UnityServiceManager::initListOfCollection);
        return;
    }
    unreadMail(model);
    if (mSystemTray) {
        mSystemTray->updateStatus(mCount);
    }

    // qCDebug(KMAIL_LOG)<<" mCount :"<<mCount;
    updateCount();
}

void UnityServiceManager::slotCollectionStatisticsChanged(Akonadi::Collection::Id id, const Akonadi::CollectionStatistics &)
{
    // Exclude sent mail folder

    if (CommonKernel->outboxCollectionFolder().id() == id || CommonKernel->sentCollectionFolder().id() == id
        || CommonKernel->templatesCollectionFolder().id() == id || CommonKernel->trashCollectionFolder().id() == id
        || CommonKernel->draftsCollectionFolder().id() == id || CommonKernel->spamsCollectionFolder().id() == id) {
        return;
    }
    initListOfCollection();
}

void UnityServiceManager::updateCount()
{
    if (mSystemTray) {
        mSystemTray->updateCount(mCount);
    }

    const int unreadEmail = KMailSettings::self()->showUnreadInTaskbar() ? mCount : 0;
    qGuiApp->setBadgeNumber(unreadEmail);
}

void UnityServiceManager::setSystemTryAssociatedWindow(QWindow *window)
{
    if (!mSystemTray) {
        return;
    }
    mSystemTray->setAssociatedWindow(window);
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

#include "moc_unityservicemanager.cpp"
