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
#include "kmsystemtray.h"
#include "kmfoldertree.h"
#include "kmfoldermgr.h"
#include "kmfolderimap.h"
#include "kmmainwidget.h"

#include <kapplication.h>
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <kiconeffect.h>
#include <kwin.h>
#include <kdebug.h>

#include <qpainter.h>
#include <qtooltip.h>
#include <qwidgetlist.h>
#include <qobjectlist.h>

#include <math.h>

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

  mDefaultIcon = SmallIcon( "kmail" );
  mTransparentIcon = SmallIcon( "kmail" );
  KIconEffect::semiTransparent( mTransparentIcon );

  setPixmap(mDefaultIcon);
  mParentVisible = true;
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
  if (!getKMMainWidget())
    return;

  mPopupMenu->insertTitle(*(this->pixmap()), "KMail");
  getKMMainWidget()->action("check_mail")->plug(mPopupMenu);
  getKMMainWidget()->action("check_mail_in")->plug(mPopupMenu);
  mPopupMenu->insertSeparator();
  getKMMainWidget()->action("new_message")->plug(mPopupMenu);
  getKMMainWidget()->action("kmail_configure_kmail")->plug(mPopupMenu);
  mPopupMenu->insertSeparator();
  if (getKMMainWidget()->action("file_quit"))
    getKMMainWidget()->action("file_quit")->plug(mPopupMenu);
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

    if(isHidden()) show();
  } else
  {
    if(mCount == 0)
    {
      hide();
    }
  }
}

/**
 * Update the count of unread messages.  If there are unread messages,
 * overlay the count on top of a transparent version of the KMail icon.
 * If there is no unread mail, restore the normal KMail icon.
 */
void KMSystemTray::updateCount()
{
  if(mCount != 0)
  {

    int oldPixmapWidth = pixmap()->size().width();
    int oldPixmapHeight = pixmap()->size().height();

    /** Scale the font size down with each increase in the number
     * of digits in the count, to a minimum point size of 6 */
    int numDigits = ((int) log10((double) mCount) + 1);
    QFont countFont = KGlobalSettings::generalFont();
    countFont.setBold(true);

    int countFontSize = countFont.pointSize();
    while(countFontSize > 5)
    {
      QFontMetrics qfm( countFont );
      int fontWidth = qfm.width( QChar( '0' ) );
      if((fontWidth * numDigits) > oldPixmapWidth)
      {
        --countFontSize;
        countFont.setPointSize(countFontSize);
      } else break;
    }

    QPixmap bg(oldPixmapWidth, oldPixmapHeight);
    bg.fill(this, 0, 0);

    /** Overlay count on transparent icon */
    QPainter p(&bg);
    p.drawPixmap(0, 0, mTransparentIcon);
    p.setFont(countFont);
    p.setPen(Qt::blue);
    p.drawText(pixmap()->rect(), Qt::AlignCenter, QString::number(mCount));

    setPixmap(bg);
  } else
  {
    setPixmap(mDefaultIcon);
  }
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
  mCount = 0;

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
 * On left mouse click, switch focus to the first KMMainWidget.  On right
 * click, bring up a list of all folders with a count of unread messages.
 */
void KMSystemTray::mousePressEvent(QMouseEvent *e)
{
  // switch to kmail on left mouse button
  if( e->button() == LeftButton )
  {
    if( mParentVisible )
      hideKMail();
    else
      showKMail();
  }

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
 * Shows and raises the first KMMainWidget and
 * switches to the appropriate virtual desktop.
 */
void KMSystemTray::showKMail()
{
  if (!getKMMainWidget())
    return;
  QWidget *mainWin = getKMMainWidget()->topLevelWidget();
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
  if (!getKMMainWidget())
    return;
  QWidget *mainWin = getKMMainWidget()->topLevelWidget();
  assert(mainWin);
  if(mainWin)
  {
    mainWin->hide();
    mParentVisible = false;
  }
}

/**
 * Grab a pointer to the first KMMainWidget.
 */
KMMainWidget * KMSystemTray::getKMMainWidget()
{
  QWidgetList *l = kapp->topLevelWidgets();
  QWidgetListIt it( *l );
  QWidget *wid;

  while ( (wid = it.current()) != 0 ) {
    ++it;
    QObjectList *l2 = wid->topLevelWidget()->queryList("KMMainWidget");
    if (l2->first())
      return dynamic_cast<KMMainWidget *>(l2->first());
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

  QMap<QGuardedPtr<KMFolder>, int>::Iterator it =
  mFoldersWithUnread.find(fldr);
  bool unmapped = (it == mFoldersWithUnread.end());

  /** If the folder is not mapped yet, increment count by numUnread
      in folder */
  if(unmapped) mCount += unread;
  /* Otherwise, get the difference between the numUnread in the folder and
   * our last known version, and adjust mCount with that difference */
  else
  {
    int diff = unread - it.data();
    mCount += diff;
  }

  if(unread > 0)
  {
    /** Add folder to our internal store, or update unread count if already mapped */
    mFoldersWithUnread.insert(fldr, unread);
    kdDebug(5006) << "There are now " << mFoldersWithUnread.count() << " folders with unread" << endl;
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

        mCount = 0;

        if(mMode == OnNewMail)
          hide();
      }
    }
  }

  updateCount();

  /** Update tooltip to reflect count of unread messages */
  QToolTip::add(this, i18n("There is 1 unread message.",
                           "There are %n unread messages.",
                           mCount));
}

/**
 * Called when user selects a folder name from the popup menu.  Shows
 * the first KMMainWin in the memberlist and jumps to the first unread
 * message in the selected folder.
 */
void KMSystemTray::selectedAccount(int id)
{
  showKMail();

  KMMainWidget * mainWidget = getKMMainWidget();
  if (!mainWidget)
  {
    kernel->openReader();
    mainWidget = getKMMainWidget();
  }

  assert(mainWidget);

  /** Select folder */
  KMFolder * fldr = mPopupFolders.at(id);
  if(!fldr) return;
  KMFolderTree * ft = mainWidget->folderTree();
  if(!ft) return;
  QListViewItem * fldrIdx = ft->indexOfFolder(fldr);
  if(!fldrIdx) return;

  ft->setCurrentItem(fldrIdx);
  ft->selectCurrentFolder();
  mainWidget->folderSelectedUnread(fldr);
}


#include "kmsystemtray.moc"
