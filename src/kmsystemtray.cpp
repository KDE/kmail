/*
   SPDX-FileCopyrightText: 2001 by Ryan Breen <ryan@porivo.com>
   SPDX-FileCopyrightText: 2010-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kmsystemtray.h"
#include "kmmainwidget.h"
#include "settings/kmailsettings.h"
#include "unityservicemanager.h"
#include <Akonadi/KMime/NewMailNotifierAttribute>
#include <MailCommon/FolderTreeView>
#include <MailCommon/MailKernel>
#include <MailCommon/MailUtil>

#include "kmail_debug.h"
#include <KLocalizedString>
#include <KWindowSystem>
#include <QMenu>

#include "widgets/kactionmenutransport.h"

using namespace MailCommon;

/**
 * Construct a KSystemTray icon to be displayed when new mail
 * has arrived in a non-system folder.  The KMSystemTray listens
 * for updateNewMessageNotification events from each non-system
 * KMFolder and maintains a store of all folders with unread
 * messages.
 *
 * The KMSystemTray also provides a popup menu listing each folder
 * with its count of unread messages, allowing the user to jump
 * to the first unread message in each folder.
 */
using namespace KMail;

KMSystemTray::KMSystemTray(QObject *parent)
    : KStatusNotifierItem(parent)
{
    qCDebug(KMAIL_LOG) << "Initting systray";
    setToolTipTitle(i18n("KMail"));
    setToolTipIconByName(QStringLiteral("kmail"));
    setIconByName(QStringLiteral("kmail"));

    KMMainWidget *mainWidget = kmkernel->getKMMainWidget();
    if (mainWidget) {
        QWidget *mainWin = mainWidget->window();
        if (mainWin) {
            mDesktopOfMainWin = KWindowInfo(mainWin->winId(), NET::WMDesktop).desktop();
        }
    }

    connect(this, &KMSystemTray::activateRequested, this, &KMSystemTray::slotActivated);
    connect(contextMenu(), &QMenu::aboutToShow, this, &KMSystemTray::slotContextMenuAboutToShow);
}

bool KMSystemTray::buildPopupMenu()
{
    KMMainWidget *mainWidget = kmkernel->getKMMainWidget();
    if (!mainWidget || kmkernel->shuttingDown()) {
        return false;
    }

    if (mBuiltContextMenu) {
        return true;
    }

    contextMenu()->clear();
    contextMenu()->setIcon(qApp->windowIcon());
    contextMenu()->setTitle(i18n("KMail"));
    QAction *action = nullptr;
    if ((action = mainWidget->action(QStringLiteral("check_mail")))) {
        contextMenu()->addAction(action);
    }
    if ((action = mainWidget->action(QStringLiteral("check_mail_in")))) {
        contextMenu()->addAction(action);
    }

    mSendQueued = mainWidget->sendQueuedAction();
    contextMenu()->addAction(mSendQueued);
    contextMenu()->addAction(mainWidget->sendQueueViaMenu());

    mNewMessagesPopup = new QMenu();
    mNewMessagesPopup->setTitle(i18n("New Messages In"));
    mNewMessagesPopup->setEnabled(false);
    connect(mNewMessagesPopup, &QMenu::triggered, this, &KMSystemTray::slotSelectCollection);
    contextMenu()->addAction(mNewMessagesPopup->menuAction());

    contextMenu()->addSeparator();
    if ((action = mainWidget->action(QStringLiteral("new_message")))) {
        contextMenu()->addAction(action);
    }
    if ((action = mainWidget->action(QStringLiteral("kmail_configure_kmail")))) {
        contextMenu()->addAction(action);
    }
    contextMenu()->addSeparator();
    if ((action = mainWidget->action(QStringLiteral("akonadi_work_offline")))) {
        contextMenu()->addAction(action);
        contextMenu()->addSeparator();
    }

    if ((action = mainWidget->action(QStringLiteral("file_quit")))) {
        contextMenu()->addAction(action);
    }

    mBuiltContextMenu = true;
    return true;
}

KMSystemTray::~KMSystemTray()
{
    delete mNewMessagesPopup;
}

void KMSystemTray::initialize(int count)
{
    updateCount(count);
    updateStatus(count);
    updateToolTip(count);
}

/**
 * Update the count of unread messages.  If there are unread messages,
 * show the "unread new mail" KMail icon.
 * If there is no unread mail, restore the normal KMail icon.
 */
void KMSystemTray::updateCount(int count)
{
    if (count == 0) {
        setIconByName(QStringLiteral("kmail"));
    } else {
        setIconByName(QStringLiteral("mail-mark-unread-new"));
    }
}

void KMSystemTray::setUnityServiceManager(UnityServiceManager *unityServiceManager)
{
    mUnityServiceManager = unityServiceManager;
}

/**
 * On left mouse click, switch focus to the first KMMainWidget.  On right
 * click, bring up a list of all folders with a count of unread messages.
 */
void KMSystemTray::slotActivated()
{
    KMMainWidget *mainWidget = kmkernel->getKMMainWidget();
    if (!mainWidget) {
        return;
    }

    QWidget *mainWin = mainWidget->window();
    if (!mainWin) {
        return;
    }

    KWindowInfo cur = KWindowInfo(mainWin->winId(), NET::WMDesktop);

    const int currentDesktop = KWindowSystem::currentDesktop();
    const bool wasMinimized = cur.isMinimized();

    if (cur.valid()) {
        mDesktopOfMainWin = cur.desktop();
    }

    if (wasMinimized && (currentDesktop != mDesktopOfMainWin) && (mDesktopOfMainWin == NET::OnAllDesktops)) {
        KWindowSystem::setOnDesktop(mainWin->winId(), currentDesktop);
    }

    if (mDesktopOfMainWin == NET::OnAllDesktops) {
        KWindowSystem::setOnAllDesktops(mainWin->winId(), true);
    }

    KWindowSystem::activateWindow(mainWin->winId());

    if (wasMinimized) {
        kmkernel->raise();
    }
}

void KMSystemTray::slotContextMenuAboutToShow()
{
    // Repeat this check before showing the menu, to minimize a race condition if
    // the base KMainWidget is closed.
    if (!kmkernel->getKMMainWidget() || kmkernel->shuttingDown()) {
        return;
    }

    // Create the context menu the first time.
    if (!mBuiltContextMenu) {
        if (!buildPopupMenu()) {
            return;
        }
    }

    // Update the "New messages in" submenu.
    mHasUnreadMessage = false;
    mNewMessagesPopup->clear();
    fillFoldersMenu(mNewMessagesPopup, kmkernel->treeviewModelSelection());
    mNewMessagesPopup->setEnabled(mHasUnreadMessage);
}

void KMSystemTray::fillFoldersMenu(QMenu *menu, const QAbstractItemModel *model, const QString &parentName, const QModelIndex &parentIndex)
{
    const int rowCount = model->rowCount(parentIndex);
    for (int row = 0; row < rowCount; ++row) {
        const QModelIndex index = model->index(row, 0, parentIndex);
        const auto collection = model->data(index, Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
        qint64 count = 0;
        if (mUnityServiceManager && !mUnityServiceManager->excludeFolder(collection)) {
            const Akonadi::CollectionStatistics statistics = collection.statistics();
            count = qMax(0LL, statistics.unreadCount());
            if (count > 0) {
                if (mUnityServiceManager->ignoreNewMailInFolder(collection)) {
                    count = 0;
                } else {
                    mHasUnreadMessage = true;
                }
            }
        }
        QString label = parentName.isEmpty() ? QString() : QString(parentName + QLatin1String("->"));
        label += model->data(index).toString();
        label.replace(QLatin1Char('&'), QStringLiteral("&&"));
        if (count > 0) {
            // insert an item
            QAction *action = menu->addAction(label);
            action->setData(collection.id());
        }
        if (model->rowCount(index) > 0) {
            fillFoldersMenu(menu, model, label, index);
        }
    }
}

void KMSystemTray::hideKMail()
{
    KMMainWidget *mainWidget = kmkernel->getKMMainWidget();
    if (!mainWidget) {
        return;
    }
    QWidget *mainWin = mainWidget->window();
    Q_ASSERT(mainWin);
    if (mainWin) {
        mDesktopOfMainWin = KWindowInfo(mainWin->winId(), NET::WMDesktop).desktop();
        // iconifying is unnecessary, but it looks cooler
        KWindowSystem::minimizeWindow(mainWin->winId());
        mainWin->hide();
    }
}

void KMSystemTray::updateToolTip(int count)
{
    setToolTipSubTitle(count == 0 ? i18n("There are no unread messages") : i18np("1 unread message", "%1 unread messages", count));
}

void KMSystemTray::updateStatus(int count)
{
    if (status() == KStatusNotifierItem::Passive && (count > 0)) {
        setStatus(KStatusNotifierItem::Active);
    } else if (status() == KStatusNotifierItem::Active && (count == 0)) {
        setStatus(KStatusNotifierItem::Passive);
    }
}

void KMSystemTray::slotSelectCollection(QAction *act)
{
    const auto id = act->data().value<Akonadi::Collection::Id>();
    kmkernel->selectCollectionFromId(id);
    KMMainWidget *mainWidget = kmkernel->getKMMainWidget();
    if (!mainWidget) {
        return;
    }
    QWidget *mainWin = mainWidget->window();
    if (mainWin && !mainWin->isVisible()) {
        activate();
    }
}
