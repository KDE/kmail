/***************************************************************************
                          kmsystemtray.cpp  -  description
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

#ifdef HAVE_CONFIG_H
  #include <config.h>
#endif

#include <kpopupmenu.h>

#include <kdebug.h>
#include <kiconloader.h>
#include <kwin.h>

#include <qevent.h>
#include <qlistview.h>
#include <qstring.h>
#include <qtooltip.h>

#include <qimage.h>
#include <qpixmap.h>
#include <qpixmapcache.h>

#include "kaction.h"
#include "kmmainwin.h"
#include "kmsystemtray.h"
#include "kmfolder.h"
#include "kmfoldertree.h"
#include "kmkernel.h"
#include "kmfoldermgr.h"
#include "kmfolderimap.h"
#include "kmmainwidget.h"

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
KMSystemTray::KMSystemTray(QWidget *parent, const char *name) : KSystemTray(parent, name),
  mNewMessagePopupId(-1), mPopupMenu(0)
{
  setAlignment( AlignCenter );
  kdDebug(5006) << "Initting systray" << endl;
  KIconLoader *loader = KGlobal::iconLoader();

  mDefaultIcon = loader->loadIcon("kmail", KIcon::Small);
  setPixmap(mDefaultIcon);
  mParentVisible = true;
  mStep = 0;
  mMode = OnNewMail;

  /** Initiate connections between folders and this object */
  foldersChanged();

  connect( kernel->folderMgr(), SIGNAL(changed()), SLOT(foldersChanged()));
  connect( kernel->imapFolderMgr(), SIGNAL(changed()), SLOT(foldersChanged()));
  connect( kernel->searchFolderMgr(), SIGNAL(changed()), SLOT(foldersChanged()));
}

void KMSystemTray::buildPopupMenu() 
{
  // Delete any previously created popup menu
  delete mPopupMenu;
  mPopupMenu = 0;

  mPopupMenu = new KPopupMenu();
  mPopupMenu->insertTitle(*(this->pixmap()), "KMail");
  getKMMainWin()->action("check_mail")->plug(mPopupMenu);
  getKMMainWin()->action("check_mail_in")->plug(mPopupMenu);
  mPopupMenu->insertSeparator();
  getKMMainWin()->action("new_message")->plug(mPopupMenu);
  getKMMainWin()->action("kmail_configure_kmail")->plug(mPopupMenu);
  mPopupMenu->insertSeparator();
  getKMMainWin()->action("file_quit")->plug(mPopupMenu);
}

KMSystemTray::~KMSystemTray()
{
  delete mPopupMenu;
  mPopupMenu = 0;
}

void KMSystemTray::setMode(int newMode)
{
  if(newMode == mMode) return;

  kdDebug(5006) << "Setting systray mMode to " << newMode << endl;
  mMode = newMode;

  if(mMode == AlwaysOn)
  {
    kdDebug(5006) << "Initting alwayson mMode" << endl;

    mAnimating = false;
    mInverted = false;
    if(isHidden()) show();
    // Already visible, so there must be new mail.  Start mAnimating
    else startAnimation();
  } else
  {
    // Animating, so there must be new mail.  Turn off animation
    if(mAnimating) mAnimating = false;
    else hide();
  }
}

void KMSystemTray::startAnimation()
{
  mAnimating = true;
  clear();
  kdDebug(5006) << "Called start animate" << endl;
  slotAnimate();
}

void KMSystemTray::slotAnimate()
{

  // Swap the icons and check again in half a second as long
  // as the user has not signalled
  // to stop animation by clicking on the system tray
  if( mAnimating )
  {
    switchIcon();
    QTimer::singleShot( 70, this, SLOT(slotAnimate()));
  } else
  {
    // Animation is over, so start polling again
    kdDebug(5006) << "User interrupted animation, poll" << endl;

    // Switch back to the default icon since animation is done
    setPixmap(mDefaultIcon);
  }
}

/**
 * Switch the Pixmap to the next icon in the sequence, moving
 * to progressively larger images before cycling back to the
 * default.  QPixmapCache is used to store the images so building
 * the pixmap is only a first time hit.
 */
void KMSystemTray::switchIcon()
{
    //kdDebug(5006) << "step is " << mStep << endl;

  QString iconName = QString("icon%1").arg(mStep);
  QPixmap icon;

  if ( !QPixmapCache::find(iconName, icon) ) {
    int w = mDefaultIcon.width() + (mStep * 1);
    int h = mDefaultIcon.width() + (mStep * 1);

    // Scale icon out from default
    icon.convertFromImage(mDefaultIcon.convertToImage().smoothScale(w, h));

    QPixmapCache::insert(iconName, icon);
  }

  if(mStep == 9) mInverted = true;
  if(mStep == 0) mInverted = false;

  if(mInverted) --mStep;
  else ++mStep;

  setPixmap(icon);
}

/**
 * Refreshes the list of folders we are monitoring.  This is called on
 * startup and is also connected to the 'changed' signal on the KMFolderMgr.
 */
void KMSystemTray::foldersChanged()
{
  /**
   * Hide and remove all unread mappings to cover the case where the only
   * unread message was in a folder that was just removed.
   */
  mFoldersWithUnread.clear();

  if(mMode == OnNewMail)
  {
    hide();
  }

  /** Disconnect all previous connections */
  disconnect(this, SLOT(updateNewMessageNotification(KMFolder *)));

  QStringList folderNames;
  QValueList<QGuardedPtr<KMFolder> > folderList;
  kernel->folderMgr()->createFolderList(&folderNames, &folderList);
  kernel->imapFolderMgr()->createFolderList(&folderNames, &folderList);
  kernel->searchFolderMgr()->createFolderList(&folderNames, &folderList);

  QStringList::iterator strIt = folderNames.begin();

  for(QValueList<QGuardedPtr<KMFolder> >::iterator it = folderList.begin();
     it != folderList.end() && strIt != folderNames.end(); ++it, ++strIt)
  {
    KMFolder * currentFolder = *it;
    QString currentName = *strIt;

    if((!currentFolder->isSystemFolder() || (currentFolder->name().lower() == "inbox")) ||
       (currentFolder->protocol() == "imap"))
    {
      /** If this is a new folder, start listening for messages */
      connect(currentFolder, SIGNAL(numUnreadMsgsChanged(KMFolder *)),
              this, SLOT(updateNewMessageNotification(KMFolder *)));

      /** Check all new folders to see if we started with any new messages */
      updateNewMessageNotification(currentFolder);
    }
  }
}

/**
 * On left mouse click, switch focus to the first KMMainWin.  On right click,
 * bring up a list of all folders with a count of unread messages.
 */
void KMSystemTray::mousePressEvent(QMouseEvent *e)
{
  // switch to kmail on left mouse button
  if( e->button() == LeftButton )
  {
    if ( mParentVisible && !mAnimating )
      hideKMail();
    else
      showKMail();
  }

  mAnimating = false;

  // open popup menu on right mouse button
  if( e->button() == RightButton )
  {
    mPopupFolders.clear();
    mPopupFolders.resize(mFoldersWithUnread.count());

    // Rebuild popup menu at click time to minimize race condition if
    // the base KMainWidget is closed.
    buildPopupMenu();

    if(mNewMessagePopupId != -1)
    {
      mPopupMenu->removeItem(mNewMessagePopupId);
    }

    if(mFoldersWithUnread.count() > 0)
    {
      KPopupMenu *newMessagesPopup = new KPopupMenu();

      QMap<QGuardedPtr<KMFolder>, int>::Iterator it = mFoldersWithUnread.begin();
      for(uint i=0; it != mFoldersWithUnread.end(); ++i)
      {
        kdDebug(5006) << "Adding folder" << endl;
        if(i > mPopupFolders.size()) mPopupFolders.resize(i * 2);
        mPopupFolders.insert(i, it.key());
        QString item = prettyName(it.key()) + "(" + QString::number(it.data()) + ")";
        newMessagesPopup->insertItem(item, this, SLOT(selectedAccount(int)), 0, i);
        ++it;
      }

      mNewMessagePopupId = mPopupMenu->insertItem(i18n("New messages in..."), 
                                                  newMessagesPopup, mNewMessagePopupId, 3); 

      kdDebug(5006) << "Folders added" << endl;
    }

    mPopupMenu->popup(e->globalPos());
  }

}

/**
 * Return the name of the folder in which the mail is deposited, prepended
 * with the account name if the folder is IMAP.
 */
QString KMSystemTray::prettyName(KMFolder * fldr)
{
  QString rvalue = fldr->label();
  if(fldr->protocol() == "imap")
  {
    KMFolderImap * imap = dynamic_cast<KMFolderImap*> (fldr);
    assert(imap);

    if((imap->account() != 0) &&
       (imap->account()->name() != 0) )
    {
      kdDebug(5006) << "IMAP folder, prepend label with type" << endl;
      rvalue = imap->account()->name() + "->" + rvalue;
    }
  }

  kdDebug(5006) << "Got label " << rvalue << endl;

  return rvalue;
}

/**
 * Shows and raises the first KMMainWin in the KMainWindow memberlist and
 * switches to the appropriate virtual desktop.
 */
void KMSystemTray::showKMail()
{
  KMMainWin * mainWin = getKMMainWin();
  assert(mainWin);
  if(mainWin)
  {
    /** Force window to grab focus */
    mainWin->show();
    mainWin->raise();
    mParentVisible = true;
    /** Switch to appropriate desktop */
    int desk = KWin::info(mainWin->winId()).desktop;
    KWin::setCurrentDesktop(desk);
  }
}

void KMSystemTray::hideKMail()
{

  KMMainWin * mainWin = getKMMainWin();
  assert(mainWin);
  if(mainWin)
  {
    mainWin->hide();
    mParentVisible = false;
  }
}

/**
 * Grab a pointer to the first KMMainWin in the KMainWindow memberlist.  Note
 * that this code may not necessarily be safe from race conditions since the
 * returned KMMainWin may have been closed by other events between the time that
 * this pointer is retrieved and used.
 */
KMMainWin * KMSystemTray::getKMMainWin()
{
  KMainWindow *kmWin = 0;

  for(kmWin = KMainWindow::memberList->first(); kmWin;
     kmWin = KMainWindow::memberList->next())
    if(kmWin->isA("KMMainWin")) break;
  if(kmWin && kmWin->isA("KMMainWin"))
  {
    return static_cast<KMMainWin *> (kmWin);
  }

  return 0;
}

/**
 * Called on startup of the KMSystemTray and when the numUnreadMsgsChanged signal
 * is emitted for one of the watched folders.  Shows the system tray icon if there
 * are new messages and the icon was hidden, or hides the system tray icon if there
 * are no more new messages.
 */
void KMSystemTray::updateNewMessageNotification(KMFolder * fldr)
{

  if(!fldr)
  {
    kdDebug(5006) << "Null folder, can't mess with that" << endl;
    return;
  }

  /** The number of unread messages in that folder */
  int unread = fldr->countUnread();

  bool unmapped = (mFoldersWithUnread.find(fldr) == mFoldersWithUnread.end());

  if(unread > 0)
  {
    /** Add folder to our internal store, or update unread count if already mapped */
    mFoldersWithUnread.insert(fldr, unread);
  }

  /**
   * Look for folder in the list of folders already represented.  If there are
   * unread messages and the system tray icon is hidden, show it.  If there are
   * no unread messages, remove the folder from the mapping.
   */
  if(unmapped)
  {
    /** Spurious notification, ignore */
    if(unread == 0) return;

    /** Make sure the icon will be displayed */
    if(mMode == OnNewMail)
    {
      if(isHidden()) show();
    } else
    {
      if(!mAnimating) startAnimation();
    }

  } else
  {

    if(unread == 0)
    {
      kdDebug(5006) << "Removing folder " << fldr->name() << endl;

      /** Remove the folder from the internal store */
      mFoldersWithUnread.remove(fldr);

      /** if this was the last folder in the dictionary, hide the systemtray icon */
      if(mFoldersWithUnread.count() == 0)
      {
        mPopupFolders.clear();
        disconnect(this, SLOT(selectedAccount(int)));

        if(mMode == OnNewMail)
          hide();
        else
          mAnimating = false;
      }
    }
  }

  /** Update tooltip to reflect count of folders with unread messages */
  int folderCount = mFoldersWithUnread.count();

  QToolTip::add(this, i18n("There is 1 folder with unread messages.",
                           "There are %n folders with unread messages.",
                           folderCount));
}

/**
 * Called when user selects a folder name from the popup menu.  Shows
 * the first KMMainWin in the memberlist and jumps to the first unread
 * message in the selected folder.
 */
void KMSystemTray::selectedAccount(int id)
{
  showKMail();

  KMMainWin * mainWin = getKMMainWin();
  assert(mainWin);

  /** Select folder */
  KMFolder * fldr = mPopupFolders.at(id);
  if(!fldr) return;
  KMFolderTree * ft = mainWin->mainKMWidget()->folderTree();
  if(!ft) return;
  QListViewItem * fldrIdx = ft->indexOfFolder(fldr);
  if(!fldrIdx) return;

  ft->setCurrentItem(fldrIdx);
  ft->selectCurrentFolder();
  mainWin->mainKMWidget()->folderSelectedUnread(fldr);
}


#include "kmsystemtray.moc"
