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
class KMSystemTray : public KStatusNotifierItem
{
    Q_OBJECT
public:
    /** construtor */
    explicit KMSystemTray(QObject *parent = nullptr);
    /** destructor */
    ~KMSystemTray();

    /**
     * Use this method to disable any systray icon changing.
     * By default this is enabled and you'll see the "new e-mail" icon whenever there's
     * new e-mail.
     */
    void setSystrayIconNotificationsEnabled(bool enable);

    void hideKMail();
    bool hasUnreadMail() const;

    void updateSystemTray();

private:
    void slotActivated();
    void slotContextMenuAboutToShow();
    void slotSelectCollection(QAction *act);
    void initListOfCollection();
    void slotCollectionStatisticsChanged(Akonadi::Collection::Id, const Akonadi::CollectionStatistics &);
    void slotGeneralFontChanged();

    bool mainWindowIsOnCurrentDesktop();
    bool buildPopupMenu();
    void updateCount();
    void fillFoldersMenu(QMenu *menu, const QAbstractItemModel *model, const QString &parentName = QString(), const QModelIndex &parentIndex = QModelIndex());
    void unreadMail(const QAbstractItemModel *model, const QModelIndex &parentIndex = QModelIndex());
    bool excludeFolder(const Akonadi::Collection &collection) const;
    bool ignoreNewMailInFolder(const Akonadi::Collection &collection);
    int mDesktopOfMainWin = 0;

    int mCount = 0;

    bool mIconNotificationsEnabled = true;

    QMenu *mNewMessagesPopup = nullptr;
    QAction *mSendQueued = nullptr;
};
}
#endif
