// -*- mode: C++; c-file-style: "gnu" -*-
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

#include <config.h>

#include "kmsystemtray.h"
#include "kmfoldertree.h"
#include "kmfoldermgr.h"
#include "kmfolderimap.h"
#include "kmmainwidget.h"
#include "kmacctmgr.h"
#include "globalsettings.h"

#include <kapplication.h>
#include <kmainwindow.h>
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <kiconeffect.h>
#include <kwin.h>
#include <kdebug.h>

#include <qpainter.h>
#include <qbitmap.h>
#include <qtooltip.h>
#include <qwidgetlist.h>
#include <qobjectlist.h>

#include <math.h>
#include <assert.h>

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
KMSystemTray::KMSystemTray(QWidget *parent, const char *name)
  : KSystemTray( parent, name ),
    mParentVisible( true ),
    mPosOfMainWin( 0, 0 ),
    mDesktopOfMainWin( 0 ),
    mMode( GlobalSettings::EnumSystemTrayPolicy::ShowOnUnread ),
    mCount( 0 ),
    mNewMessagePopupId(-1),
    mPopupMenu(0)
{
  setAlignment( AlignCenter );
  kdDebug(5006) << "Initting systray" << endl;

  mLastUpdate = time( 0 );
  mUpdateTimer = new QTimer( this, "systraytimer" );
  connect( mUpdateTimer, SIGNAL( timeout() ), SLOT( updateNewMessages() ) );

  mDefaultIcon = loadIcon( "kmail" );
  mLightIconImage = loadIcon( "kmaillight" ).convertToImage();

  setPixmap(mDefaultIcon);

  KMMainWidget * mainWidget = getKMMainWidget();
  if ( mainWidget ) {
    QWidget * mainWin = mainWidget->topLevelWidget();
    if ( mainWin ) {
      mDesktopOfMainWin = KWin::windowInfo( mainWin->winId(),
                                            NET::WMDesktop ).desktop();
      mPosOfMainWin = mainWin->pos();
    }
  }

  // register the applet with the kernel
  kmkernel->registerSystemTrayApplet( this );

  /** Initiate connections between folders and this object */
  foldersChanged();

  connect( kmkernel->folderMgr(), SIGNAL(changed()), SLOT(foldersChanged()));
  connect( kmkernel->imapFolderMgr(), SIGNAL(changed()), SLOT(foldersChanged()));
  connect( kmkernel->dimapFolderMgr(), SIGNAL(changed()), SLOT(foldersChanged()));
  connect( kmkernel->searchFolderMgr(), SIGNAL(changed()), SLOT(foldersChanged()));

  connect( kmkernel->acctMgr(), SIGNAL( checkedMail( bool, bool, const QMap<QString, int> & ) ),
           SLOT( updateNewMessages() ) );
}

void KMSystemTray::buildPopupMenu()
{
  // Delete any previously created popup menu
  delete mPopupMenu;
  mPopupMenu = 0;

  mPopupMenu = new KPopupMenu();
  KMMainWidget * mainWidget = getKMMainWidget();
  if ( !mainWidget )
    return;

  mPopupMenu->insertTitle(*(this->pixmap()), "KMail");
  KAction * action;
  if ( ( action = mainWidget->action("check_mail") ) )
    action->plug( mPopupMenu );
  if ( ( action = mainWidget->action("check_mail_in") ) )
    action->plug( mPopupMenu );
  if ( ( action = mainWidget->action("send_queued") ) )
    action->plug( mPopupMenu );
  mPopupMenu->insertSeparator();
  if ( ( action = mainWidget->action("new_message") ) )
    action->plug( mPopupMenu );
  if ( ( action = mainWidget->action("kmail_configure_kmail") ) )
    action->plug( mPopupMenu );
  mPopupMenu->insertSeparator();

  KMainWindow *mainWin = ::qt_cast<KMainWindow*>(getKMMainWidget()->topLevelWidget());
  if(mainWin)
    if ( ( action=mainWin->actionCollection()->action("file_quit") ) )
      action->plug( mPopupMenu );
}

KMSystemTray::~KMSystemTray()
{
  // unregister the applet
  kmkernel->unregisterSystemTrayApplet( this );

  delete mPopupMenu;
  mPopupMenu = 0;
}

void KMSystemTray::setMode(int newMode)
{
  if(newMode == mMode) return;

  kdDebug(5006) << "Setting systray mMode to " << newMode << endl;
  mMode = newMode;

  switch ( mMode ) {
  case GlobalSettings::EnumSystemTrayPolicy::ShowAlways:
    if ( isHidden() )
      show();
    break;
  case GlobalSettings::EnumSystemTrayPolicy::ShowOnUnread:
    if ( mCount == 0 && !isHidden() )
      hide();
    else if ( mCount > 0 && isHidden() )
      show();
  default:
    kdDebug(5006) << k_funcinfo << " Unknown systray mode " << mMode << endl;
  }
}

int KMSystemTray::mode() const
{
  return mMode;
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

    QString countString = QString::number( mCount );
    QFont countFont = KGlobalSettings::generalFont();
    countFont.setBold(true);

    // decrease the size of the font for the number of unread messages if the
    // number doesn't fit into the available space
    float countFontSize = countFont.pointSizeFloat();
    QFontMetrics qfm( countFont );
    int width = qfm.width( countString );
    if( width > oldPixmapWidth )
    {
      countFontSize *= float( oldPixmapWidth ) / float( width );
      countFont.setPointSizeFloat( countFontSize );
    }

    // Create an image which represents the number of unread messages
    // and which has a transparent background.
    // Unfortunately this required the following twisted code because for some
    // reason text that is drawn on a transparent pixmap is invisible
    // (apparently the alpha channel isn't changed when the text is drawn).
    // Therefore I have to draw the text on a solid background and then remove
    // the background by making it transparent with QPixmap::setMask. This
    // involves the slow createHeuristicMask() function (from the API docs:
    // "This function is slow because it involves transformation to a QImage,
    // non-trivial computations and a transformation back to a QBitmap."). Then
    // I have to convert the resulting QPixmap to a QImage in order to overlay
    // the light KMail icon with the number (because KIconEffect::overlay only
    // works with QImage). Finally the resulting QImage has to be converted
    // back to a QPixmap.
    // That's a lot of work for overlaying the KMail icon with the number of
    // unread messages, but every other approach I tried failed miserably.
    //                                                           IK, 2003-09-22
    QPixmap numberPixmap( oldPixmapWidth, oldPixmapHeight );
    numberPixmap.fill( Qt::white );
    QPainter p( &numberPixmap );
    p.setFont( countFont );
    p.setPen( Qt::blue );
    p.drawText( numberPixmap.rect(), Qt::AlignCenter, countString );
    numberPixmap.setMask( numberPixmap.createHeuristicMask() );
    QImage numberImage = numberPixmap.convertToImage();

    // Overlay the light KMail icon with the number image
    QImage iconWithNumberImage = mLightIconImage.copy();
    KIconEffect::overlay( iconWithNumberImage, numberImage );

    QPixmap iconWithNumber;
    iconWithNumber.convertFromImage( iconWithNumberImage );
    setPixmap( iconWithNumber );
  } else
  {
    setPixmap( mDefaultIcon );
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

  if ( mMode == GlobalSettings::EnumSystemTrayPolicy::ShowOnUnread ) {
    hide();
  }

  /** Disconnect all previous connections */
  disconnect(this, SLOT(updateNewMessageNotification(KMFolder *)));

  QStringList folderNames;
  QValueList<QGuardedPtr<KMFolder> > folderList;
  kmkernel->folderMgr()->createFolderList(&folderNames, &folderList);
  kmkernel->imapFolderMgr()->createFolderList(&folderNames, &folderList);
  kmkernel->dimapFolderMgr()->createFolderList(&folderNames, &folderList);
  kmkernel->searchFolderMgr()->createFolderList(&folderNames, &folderList);

  QStringList::iterator strIt = folderNames.begin();

  for(QValueList<QGuardedPtr<KMFolder> >::iterator it = folderList.begin();
     it != folderList.end() && strIt != folderNames.end(); ++it, ++strIt)
  {
    KMFolder * currentFolder = *it;
    QString currentName = *strIt;

    if ( ((!currentFolder->isSystemFolder() || (currentFolder->name().lower() == "inbox")) ||
         (currentFolder->folderType() == KMFolderTypeImap)) &&
         !currentFolder->ignoreNewMail() )
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
    if( mParentVisible && mainWindowIsOnCurrentDesktop() )
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

      mNewMessagePopupId = mPopupMenu->insertItem(i18n("New Messages In"),
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
  if(fldr->folderType() == KMFolderTypeImap)
  {
    KMFolderImap * imap = dynamic_cast<KMFolderImap*> (fldr->storage());
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


bool KMSystemTray::mainWindowIsOnCurrentDesktop()
{
  KMMainWidget * mainWidget = getKMMainWidget();
  if ( !mainWidget )
    return false;

  QWidget *mainWin = getKMMainWidget()->topLevelWidget();
  if ( !mainWin )
    return false;

  return KWin::windowInfo( mainWin->winId(),
                           NET::WMDesktop ).isOnCurrentDesktop();
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
    // switch to appropriate desktop
    if ( mDesktopOfMainWin != NET::OnAllDesktops )
      KWin::setCurrentDesktop( mDesktopOfMainWin );
    if ( !mParentVisible ) {
      if ( mDesktopOfMainWin == NET::OnAllDesktops )
        KWin::setOnAllDesktops( mainWin->winId(), true );
      mainWin->move( mPosOfMainWin );
      mainWin->show();
    }
    KWin::activateWindow( mainWin->winId() );
    mParentVisible = true;
  }

  //Fake that the folders have changed so that the icon status is correct
  foldersChanged();
}

void KMSystemTray::hideKMail()
{
  if (!getKMMainWidget())
    return;
  QWidget *mainWin = getKMMainWidget()->topLevelWidget();
  assert(mainWin);
  if(mainWin)
  {
    mDesktopOfMainWin = KWin::windowInfo( mainWin->winId(),
                                          NET::WMDesktop ).desktop();
    mPosOfMainWin = mainWin->pos();
    // iconifying is unnecessary, but it looks cooler
    KWin::iconifyWindow( mainWin->winId() );
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
    if (l2 && l2->first())
	{
	  KMMainWidget* kmmw = dynamic_cast<KMMainWidget *>(l2->first());
	  assert (kmmw);
	  delete l2;
	  delete l;
	  return kmmw;
	}
    delete l2;
  }
  delete l;
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
  //We don't want to count messages from search folders as they
  //  already counted as part of their original folders
  if( !fldr ||
      fldr->folderType() == KMFolderTypeSearch )
  {
    // kdDebug(5006) << "Null or a search folder, can't mess with that" << endl;
    return;
  }

  mPendingUpdates[ fldr ] = true;
  if ( time( 0 ) - mLastUpdate > 2 ) {
    mUpdateTimer->stop();
    updateNewMessages();
  }
  else {
    mUpdateTimer->start(150, true);
  }
}

void KMSystemTray::updateNewMessages()
{
  for ( QMap<KMFolder*, bool>::Iterator it = mPendingUpdates.begin();
        it != mPendingUpdates.end(); ++it)
  {
  KMFolder *fldr = it.key();

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
    //kdDebug(5006) << "There are now " << mFoldersWithUnread.count() << " folders with unread" << endl;
  }

  /**
   * Look for folder in the list of folders already represented.  If there are
   * unread messages and the system tray icon is hidden, show it.  If there are
   * no unread messages, remove the folder from the mapping.
   */
  if(unmapped)
  {
    /** Spurious notification, ignore */
    if(unread == 0) continue;

    /** Make sure the icon will be displayed */
    if ( ( mMode == GlobalSettings::EnumSystemTrayPolicy::ShowOnUnread )
         && isHidden() ) {
      show();
    }

  } else
  {

    if(unread == 0)
    {
      kdDebug(5006) << "Removing folder from internal store " << fldr->name() << endl;

      /** Remove the folder from the internal store */
      mFoldersWithUnread.remove(fldr);

      /** if this was the last folder in the dictionary, hide the systemtray icon */
      if(mFoldersWithUnread.count() == 0)
      {
        mPopupFolders.clear();
        disconnect(this, SLOT(selectedAccount(int)));

        mCount = 0;

        if ( mMode == GlobalSettings::EnumSystemTrayPolicy::ShowOnUnread ) {
          hide();
        }
      }
    }
  }

  }
  mPendingUpdates.clear();
  updateCount();

  /** Update tooltip to reflect count of unread messages */
  QToolTip::remove(this);
  QToolTip::add(this, i18n("There is 1 unread message.",
                           "There are %n unread messages.",
                           mCount));

  mLastUpdate = time( 0 );
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
    kmkernel->openReader();
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
}

#include "kmsystemtray.moc"
