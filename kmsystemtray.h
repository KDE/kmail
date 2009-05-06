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

#include <ksystemtrayicon.h>

#include <QMap>
#include <QPointer>
#include <QVector>

#include <QPixmap>
#include <QImage>

#include <time.h>

class KMFolder;
class QPoint;
class QMenu;

/**
 * KMSystemTray extends KSystemTray and handles system
 * tray notification for KMail
 */
class KMSystemTray : public KSystemTrayIcon
{
  Q_OBJECT
public:
  /** construtor */
  KMSystemTray(QWidget* parent=0);
  /** destructor */
  ~KMSystemTray();

  void setMode(int mode);
  int mode() const;

  void hideKMail();
  bool hasUnreadMail() const;

public slots:
  void foldersChanged();

private slots:
  void updateNewMessageNotification(KMFolder * folder);
  void selectedAccount(int);
  void updateNewMessages();
  void slotActivated( QSystemTrayIcon::ActivationReason reason );
  void slotContextMenuAboutToShow();

protected:
  bool mainWindowIsOnCurrentDesktop();
  void showKMail();
  void buildPopupMenu();
  void updateCount();

  QString prettyName(KMFolder *);

private:

  bool mParentVisible;
  QPoint mPosOfMainWin;
  int mDesktopOfMainWin;

  int mMode;
  int mCount;

  QMenu *mNewMessagesPopup;
  QAction *mSendQueued;
  QPixmap mDefaultIcon;

  QVector<KMFolder*> mPopupFolders;
  QMap<QPointer<KMFolder>, int> mFoldersWithUnread;
  QMap<QPointer<KMFolder>, bool> mPendingUpdates;
  QTimer *mUpdateTimer;
  time_t mLastUpdate;
};

#endif
