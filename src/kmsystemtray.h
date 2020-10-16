/*
   SPDX-FileCopyrightText: 2001 by Ryan Breen <ryan@porivo.com>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KMSYSTEMTRAY_H
#define KMSYSTEMTRAY_H

#include <AkonadiCore/collection.h>
#include <KStatusNotifierItem>

#include <QAction>
#include <QAbstractItemModel>

class QMenu;

/**
 * KMSystemTray extends KStatusNotifierItem and handles system
 * tray notification for KMail
 */
namespace KMail {
class UnityServiceManager;
class KMSystemTray : public KStatusNotifierItem
{
    Q_OBJECT
public:
    /** constructor */
    explicit KMSystemTray(QObject *parent = nullptr);
    /** destructor */
    ~KMSystemTray();

    void hideKMail();

    void updateStatus(int count);
    void updateCount(int count);
    void setUnityServiceManager(KMail::UnityServiceManager *unityServiceManager);
    void initialize(int count);
    void updateToolTip(int count);

private:
    void slotActivated();
    void slotContextMenuAboutToShow();
    void slotSelectCollection(QAction *act);

    Q_REQUIRED_RESULT bool buildPopupMenu();
    void fillFoldersMenu(QMenu *menu, const QAbstractItemModel *model, const QString &parentName = QString(), const QModelIndex &parentIndex = QModelIndex());
    int mDesktopOfMainWin = 0;
    bool mBuiltContextMenu = false;

    bool mHasUnreadMessage = false;
    bool mIconNotificationsEnabled = true;

    QMenu *mNewMessagesPopup = nullptr;
    QAction *mSendQueued = nullptr;
    KMail::UnityServiceManager *mUnityServiceManager = nullptr;
};
}
#endif
