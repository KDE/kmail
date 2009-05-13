// -*- mode: C++; c-file-style: "gnu" -*-
/***************************************************************************
                          kmsystemtray.cpp  -  description
                             ------------------
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


#include "kmsystemtray.h"
#include "kmfolder.h"
#include "kmfoldertree.h"
#include "kmfoldermgr.h"
#include "kmfolderimap.h"
#include "kmmainwidget.h"
#include "accountmanager.h"

using KMail::AccountManager;
#include "globalsettings.h"

#include <kxmlguiwindow.h>
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <kcolorscheme.h>
#include <kwindowsystem.h>
#include <kdebug.h>
#include <KMenu>

#include <QPainter>
#include <QBitmap>

#include <QWidget>
#include <QObject>

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
KMSystemTray::KMSystemTray(QWidget *parent)
  : KSystemTrayIcon( parent),
    mParentVisible( true ),
    mPosOfMainWin( 0, 0 ),
    mDesktopOfMainWin( 0 ),
    mMode( GlobalSettings::EnumSystemTrayPolicy::ShowOnUnread ),
    mCount( 0 ),
    mNewMessagePopupId(-1)
{
  kDebug(5006) <<"Initting systray";

  mLastUpdate = time( 0 );
  mUpdateTimer = new QTimer( this );
  mUpdateTimer->setSingleShot( true );
  connect( mUpdateTimer, SIGNAL( timeout() ), SLOT( updateNewMessages() ) );

  mDefaultIcon = KIcon( "kmail" ).pixmap( 22 );
  setIcon( mDefaultIcon );
#ifdef Q_WS_X11
  KMMainWidget * mainWidget = kmkernel->getKMMainWidget();
  if ( mainWidget ) {
    QWidget * mainWin = mainWidget->topLevelWidget();
    if ( mainWin ) {
      mDesktopOfMainWin = KWindowSystem::windowInfo( mainWin->winId(),
                                            NET::WMDesktop ).desktop();
      mPosOfMainWin = mainWin->pos();
    }
  }
#endif
  // register the applet with the kernel
  kmkernel->registerSystemTrayApplet( this );

  /** Initiate connections between folders and this object */
  foldersChanged();

  connect( this, SIGNAL( activated( QSystemTrayIcon::ActivationReason ) ),
           this, SLOT( slotActivated( QSystemTrayIcon::ActivationReason ) ) );
  connect( contextMenu(), SIGNAL( aboutToShow() ),
           this, SLOT( slotContextMenuAboutToShow() ) );

  connect( kmkernel->folderMgr(), SIGNAL(changed()), SLOT(foldersChanged()));
  connect( kmkernel->imapFolderMgr(), SIGNAL(changed()), SLOT(foldersChanged()));
  connect( kmkernel->dimapFolderMgr(), SIGNAL(changed()), SLOT(foldersChanged()));
  connect( kmkernel->searchFolderMgr(), SIGNAL(changed()), SLOT(foldersChanged()));

  connect( kmkernel->acctMgr(), SIGNAL( checkedMail( bool, bool, const QMap<QString, int> & ) ),
           SLOT( updateNewMessages() ) );
}

void KMSystemTray::buildPopupMenu()
{
  if ( !contextMenu() ) {
    setContextMenu( new KMenu() );
  }

  contextMenu()->clear();

  KMMainWidget * mainWidget = kmkernel->getKMMainWidget();
  if ( !mainWidget )
    return;

  static_cast<KMenu*>( contextMenu() )->addTitle(this->icon(), "KMail");
  QAction * action;
  if ( ( action = mainWidget->action("check_mail") ) )
    contextMenu()->addAction( action );
  if ( ( action = mainWidget->action("check_mail_in") ) )
    contextMenu()->addAction( action );
  if ( ( action = mainWidget->action("send_queued") ) )
    contextMenu()->addAction( action );
  if ( ( action = mainWidget->action("send_queued_via") ) )
    contextMenu()->addAction( action );
  contextMenu()->addSeparator();
  if ( ( action = mainWidget->action("new_message") ) )
    contextMenu()->addAction( action );
  if ( ( action = mainWidget->action("kmail_configure_kmail") ) )
    contextMenu()->addAction( action );
  contextMenu()->addSeparator();

  KXmlGuiWindow *mainWin = ::qobject_cast<KXmlGuiWindow*>(kmkernel->getKMMainWidget()->topLevelWidget());
  if(mainWin)
    if ( ( action=mainWin->actionCollection()->action("file_quit") ) )
      contextMenu()->addAction( action );
}

KMSystemTray::~KMSystemTray()
{
  // unregister the applet
  kmkernel->unregisterSystemTrayApplet( this );
}

void KMSystemTray::setMode(int newMode)
{
  if(newMode == mMode) return;

  kDebug(5006) <<"Setting systray mMode to" << newMode;
  mMode = newMode;

  switch ( mMode ) {
  case GlobalSettings::EnumSystemTrayPolicy::ShowAlways:
    if ( !isVisible() )
      show();
    break;
  case GlobalSettings::EnumSystemTrayPolicy::ShowOnUnread:
    if ( mCount == 0 && isVisible() )
      hide();
    else if ( mCount > 0 && !isVisible() )
      show();
    break;
  default:
    kDebug(5006) <<" Unknown systray mode" << mMode;
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
    int oldPixmapWidth = mDefaultIcon.size().width();

    QString countString = QString::number( mCount );
    QFont countFont = KGlobalSettings::generalFont();
    countFont.setBold(true);

    // decrease the size of the font for the number of unread messages if the
    // number doesn't fit into the available space
    float countFontSize = countFont.pointSizeF();
    QFontMetrics qfm( countFont );
    int width = qfm.width( countString );
    if( width > (oldPixmapWidth - 2) )
    {
      countFontSize *= float( oldPixmapWidth - 2 ) / float( width );
      countFont.setPointSizeF( countFontSize );
    }

    // Overlay the light KMail icon with the number image
    QPixmap iconWithNumberImage = mDefaultIcon;
    QPainter p( &iconWithNumberImage );
    p.setFont( countFont );
    KColorScheme scheme( QPalette::Active, KColorScheme::View );

    qfm = QFontMetrics( countFont );
    QRect boundingRect = qfm.tightBoundingRect( countString );
    boundingRect.adjust( 0, 0, 0, 2 );
    boundingRect.setHeight( qMin( boundingRect.height(), oldPixmapWidth ) );
    boundingRect.moveTo( (oldPixmapWidth - boundingRect.width()) / 2,
                         ((oldPixmapWidth - boundingRect.height()) / 2) - 1 );
    p.setOpacity( 0.7 );
    p.setBrush( scheme.background( KColorScheme::LinkBackground ) );
    p.setPen( scheme.background( KColorScheme::LinkBackground ).color() );
    p.drawRoundedRect( boundingRect, 2.0, 2.0 );

    p.setBrush( Qt::NoBrush );
    p.setPen( scheme.foreground( KColorScheme::LinkText ).color() );
    p.setOpacity( 1.0 );
    p.drawText( iconWithNumberImage.rect(), Qt::AlignCenter, countString );

    setIcon( iconWithNumberImage );
  } else
  {
    setIcon( mDefaultIcon );
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
  mPendingUpdates.clear();
  mCount = 0;

  if ( mMode == GlobalSettings::EnumSystemTrayPolicy::ShowOnUnread ) {
    hide();
  }

  /** Disconnect all previous connections */
  disconnect(this, SLOT(updateNewMessageNotification(KMFolder *)));

  QStringList folderNames;
  QList<QPointer<KMFolder> > folderList;
  kmkernel->folderMgr()->createFolderList(&folderNames, &folderList);
  kmkernel->imapFolderMgr()->createFolderList(&folderNames, &folderList);
  kmkernel->dimapFolderMgr()->createFolderList(&folderNames, &folderList);
  kmkernel->searchFolderMgr()->createFolderList(&folderNames, &folderList);

  QStringList::iterator strIt = folderNames.begin();

  for(QList<QPointer<KMFolder> >::iterator it = folderList.begin();
     it != folderList.end() && strIt != folderNames.end(); ++it, ++strIt)
  {
    KMFolder * currentFolder = *it;
    QString currentName = *strIt;

    if ( ((!currentFolder->isSystemFolder() || (currentFolder->name().toLower() == "inbox")) ||
         (currentFolder->folderType() == KMFolderTypeImap)) &&
         !currentFolder->ignoreNewMail() )
    {
      /** If this is a new folder, start listening for messages */
      connect(currentFolder, SIGNAL(numUnreadMsgsChanged(KMFolder *)),
              this, SLOT(updateNewMessageNotification(KMFolder *)));

      /** Check all new folders to see if we started with any new messages */
      updateNewMessageNotification(currentFolder);
    }
    else {
        disconnect(currentFolder, SIGNAL(numUnreadMsgsChanged(KMFolder *)), this, SLOT(updateNewMessageNotification(KMFolder *)) );
    }
  }
}

/**
 * On left mouse click, switch focus to the first KMMainWidget.  On right
 * click, bring up a list of all folders with a count of unread messages.
 */
void KMSystemTray::slotActivated( QSystemTrayIcon::ActivationReason reason )
{
  kDebug(5006) <<"trigger:" << reason;

  // switch to kmail on left mouse button
  if( reason == QSystemTrayIcon::Trigger )
  {
    if( mParentVisible && mainWindowIsOnCurrentDesktop() )
      hideKMail();
    else
      showKMail();
  }
}

void KMSystemTray::slotContextMenuAboutToShow() 
{
  // Rebuild popup menu before show to minimize race condition if
  // the base KMainWidget is closed.
  buildPopupMenu();

  if(mNewMessagePopupId != -1)
  {
    contextMenu()->removeItem(mNewMessagePopupId);
  }

  if(mFoldersWithUnread.count() > 0)
  {
    KMenu *newMessagesPopup = new KMenu();

    QMap<QPointer<KMFolder>, int>::Iterator it = mFoldersWithUnread.begin();
    for(uint i=0; it != mFoldersWithUnread.end(); ++i)
    {
      kDebug(5006) <<"Adding folder";
      mPopupFolders.append( it.key() );
      QString item = prettyName(it.key()) + " (" + QString::number(it.value()) + ')';
      newMessagesPopup->insertItem(item, this, SLOT(selectedAccount(int)), 0, i);
      ++it;
    }

    mNewMessagePopupId = contextMenu()->insertItem(i18n("New Messages In"),
                                                newMessagesPopup, mNewMessagePopupId, 3);

    kDebug(5006) <<"Folders added";
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
      kDebug(5006) <<"IMAP folder, prepend label with type";
      rvalue = imap->account()->name() + "->" + rvalue;
    }
  }

  kDebug(5006) <<"Got label" << rvalue;

  return rvalue;
}


bool KMSystemTray::mainWindowIsOnCurrentDesktop()
{
#ifdef Q_WS_X11
  KMMainWidget * mainWidget = kmkernel->getKMMainWidget();
  if ( !mainWidget )
    return false;

  QWidget *mainWin = kmkernel->getKMMainWidget()->topLevelWidget();
  if ( !mainWin )
    return false;

  return KWindowSystem::windowInfo( mainWin->winId(),
                           NET::WMDesktop ).isOnCurrentDesktop();
#else
  return true;
#endif
}

/**
 * Shows and raises the first KMMainWidget and
 * switches to the appropriate virtual desktop.
 */
void KMSystemTray::showKMail()
{
  if (!kmkernel->getKMMainWidget())
    return;
  QWidget *mainWin = kmkernel->getKMMainWidget()->topLevelWidget();
  assert(mainWin);
#ifdef Q_WS_X11
  if(mainWin)
  {
    KWindowInfo cur = KWindowSystem::windowInfo( mainWin->winId(), NET::WMDesktop );
    if ( cur.valid() ) mDesktopOfMainWin = cur.desktop();
    // switch to appropriate desktop
    if ( mDesktopOfMainWin != NET::OnAllDesktops )
      KWindowSystem::setCurrentDesktop( mDesktopOfMainWin );
    if ( !mParentVisible ) {
      if ( mDesktopOfMainWin == NET::OnAllDesktops )
        KWindowSystem::setOnAllDesktops( mainWin->winId(), true );
    }
    KWindowSystem::activateWindow( mainWin->winId() );
  }
#endif
  if ( !mParentVisible ) {
    mainWin->move( mPosOfMainWin );
    mainWin->show();
    mParentVisible = true;
  }
  kmkernel->raise();

  //Fake that the folders have changed so that the icon status is correct
  foldersChanged();
}

void KMSystemTray::hideKMail()
{
  if (!kmkernel->getKMMainWidget())
    return;
  QWidget *mainWin = kmkernel->getKMMainWidget()->topLevelWidget();
  assert(mainWin);
  if(mainWin)
  {
    mPosOfMainWin = mainWin->pos();
#ifdef Q_WS_X11
    mDesktopOfMainWin = KWindowSystem::windowInfo( mainWin->winId(),
                                          NET::WMDesktop ).desktop();
    // iconifying is unnecessary, but it looks cooler
    KWindowSystem::minimizeWindow( mainWin->winId() );
#endif
    mainWin->hide();
    mParentVisible = false;
  }
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
    // kDebug(5006) <<"Null or a search folder, can't mess with that";
    return;
  }

  mPendingUpdates[ fldr ] = true;
  if ( time( 0 ) - mLastUpdate > 2 ) {
    mUpdateTimer->stop();
    updateNewMessages();
  }
  else {
    mUpdateTimer->start(150);
  }
}

void KMSystemTray::updateNewMessages()
{
  for ( QMap<QPointer<KMFolder>, bool>::Iterator it1 = mPendingUpdates.begin();
        it1 != mPendingUpdates.end(); ++it1)
  {
    KMFolder *fldr = it1.key();
    if ( !fldr ) // deleted folder
      continue;

    // The number of unread messages in that folder
    int unread = fldr->countUnread();

    QMap<QPointer<KMFolder>, int>::Iterator it = mFoldersWithUnread.find( fldr );
    bool unmapped = ( it == mFoldersWithUnread.end() );

    // If the folder is not mapped yet, increment count by numUnread
    // in folder
    if ( unmapped )
      mCount += unread;

    //Otherwise, get the difference between the numUnread in the folder and
    // our last known version, and adjust mCount with that difference
    else {
      int diff = unread - it.value();
      mCount += diff;
    }

    if ( unread > 0 ) {
      // Add folder to our internal store, or update unread count if already mapped
      mFoldersWithUnread.insert( fldr, unread );
      //kDebug(5006) <<"There are now" << mFoldersWithUnread.count() <<" folders with unread";
    }

    /*
     * Look for folder in the list of folders already represented.  If there are
     * unread messages and the system tray icon is hidden, show it.  If there are
     * no unread messages, remove the folder from the mapping.
     */
    if ( unmapped ) {

      // Spurious notification, ignore
      if ( unread == 0 )
        continue;

      // Make sure the icon will be displayed
      if ( ( mMode == GlobalSettings::EnumSystemTrayPolicy::ShowOnUnread ) &&
           !isVisible() ) {
        show();
      }
    }
    else {
      if ( unread == 0 ) {
        kDebug(5006) << "Removing folder from internal store" << fldr->name();

        // Remove the folder from the internal store
        mFoldersWithUnread.remove(fldr);

        // if this was the last folder in the dictionary, hide the systemtray icon
        if ( mFoldersWithUnread.count() == 0 ) {
          mPopupFolders.clear();
          disconnect ( this, SLOT( selectedAccount( int ) ) );
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

  // Update tooltip to reflect count of unread messages
  setToolTip( mCount == 0 ? i18n("KMail - There are no unread messages")
                          : i18np("KMail - 1 unread message",
                                  "KMail - %1 unread messages",
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

  KMMainWidget * mainWidget = kmkernel->getKMMainWidget();
  if (!mainWidget)
  {
    kmkernel->openReader();
    mainWidget = kmkernel->getKMMainWidget();
  }

  assert(mainWidget);

  /** Select folder */
  KMFolder * fldr = mPopupFolders.at(id);
  if(!fldr) return;
  KMFolderTree * ft = mainWidget->folderTree();
  if(!ft) return;
  Q3ListViewItem * fldrIdx = ft->indexOfFolder(fldr);
  if(!fldrIdx) return;

  ft->setCurrentItem(fldrIdx);
  ft->selectCurrentFolder();
}

bool KMSystemTray::hasUnreadMail() const
{
  return ( mCount != 0 );
}

#include "kmsystemtray.moc"
