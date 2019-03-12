/***************************************************************************
              kmsystemtray.h  -  description
               -------------------
  begin                : Fri Aug 31 22:38:44 EDT 2001
  copyright            : (C) 2001 by Ryan Breen
  email                : ryan@porivo.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KMSYSTEMTRAY_H
#define KMSYSTEMTRAY_H

#include <AkonadiCore/collection.h>
#include <kstatusnotifieritem.h>

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
    /** construtor */
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

    bool mainWindowIsOnCurrentDesktop();
    bool buildPopupMenu();
    void fillFoldersMenu(QMenu *menu, const QAbstractItemModel *model, const QString &parentName = QString(), const QModelIndex &parentIndex = QModelIndex());
    int mDesktopOfMainWin = 0;

    bool mHasUnreadMessage = false;
    bool mIconNotificationsEnabled = true;

    QMenu *mNewMessagesPopup = nullptr;
    QAction *mSendQueued = nullptr;
    KMail::UnityServiceManager *mUnityServiceManager = nullptr;
};
}
#endif
