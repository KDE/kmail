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

#include <ksystemtray.h>

#include <qmap.h>
#include <qguardedptr.h>
#include <qptrvector.h>
#include <qpixmap.h>

class KMFolder;
class KMFolderMgr;
class KMMainWin;
class QMouseEvent;
class KPopupMenu;

/** 
 * KMSystemTray extends KSystemTray and handles system
 * tray notification for KMail 
 */
class KMSystemTray : public KSystemTray
{
  Q_OBJECT 
  public:
  /** construtor */
  KMSystemTray(QWidget* parent=0, const char *name=0);
  /** destructor */
  ~KMSystemTray();

  void setMode(int mode);

  enum SystrayMode {
    AlwaysOn,
    OnNewMail
  };

private slots:
  void updateNewMessageNotification(KMFolder * folder);
  void foldersChanged();
  void selectedAccount(int);
  void slotAnimate();

protected:
  void mousePressEvent(QMouseEvent *);
  void showKMail();
  void hideKMail();
  void switchIcon();
  void startAnimation();
  void buildPopupMenu();

  QString prettyName(KMFolder *);
  KMMainWin * getKMMainWin();

private:

  QPixmap mDefaultIcon;
  
  bool mInverted;
  bool mAnimating;
  bool mParentVisible;

  int mMode;
  int mStep;

  int mNewMessagePopupId;

  KPopupMenu * mPopupMenu;

  QPtrVector<KMFolder> mPopupFolders;
  QMap<QGuardedPtr<KMFolder>, int> mFoldersWithUnread;
};

#endif
