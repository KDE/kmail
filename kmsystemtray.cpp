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
#include "kmmainwidget.h"

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
#include <QSignalMapper>

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
KMSystemTray::KMSystemTray(QObject *parent)
  : KStatusNotifierItem( parent),
    mParentVisible( true ),
    mPosOfMainWin( 0, 0 ),
    mDesktopOfMainWin( 0 ),
    mMode( GlobalSettings::EnumSystemTrayPolicy::ShowOnUnread ),
    mCount( 0 ),
    mNewMessagesPopup( 0 ),
    mSendQueued( 0 )
{
  kDebug() << "Initting systray";
  setToolTipTitle( "KMail" );
  setToolTipIconByName( "kmail" );
  setIconByName( "kmail" );

  mLastUpdate = time( 0 );
  mUpdateTimer = new QTimer( this );
  mUpdateTimer->setSingleShot( true );
  connect( mUpdateTimer, SIGNAL( timeout() ), SLOT( updateNewMessages() ) );

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

  connect( this, SIGNAL( activateRequested( bool, const QPoint& ) ),
           this, SLOT( slotActivated() ) );
  connect( contextMenu(), SIGNAL( aboutToShow() ),
           this, SLOT( slotContextMenuAboutToShow() ) );
#if 0
  connect( kmkernel->folderMgr(), SIGNAL(changed()), SLOT(foldersChanged()));
  connect( kmkernel->searchFolderMgr(), SIGNAL(changed()), SLOT(foldersChanged()));

#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
#if 0
  connect( kmkernel->acctMgr(), SIGNAL( checkedMail( bool, bool, const QMap<QString, int> & ) ),
           SLOT( updateNewMessages() ) );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
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

  contextMenu()->addTitle(qApp->windowIcon(), "KMail");
  QAction * action;
  if ( ( action = mainWidget->action("check_mail") ) )
    contextMenu()->addAction( action );
  if ( ( action = mainWidget->action("check_mail_in") ) )
    contextMenu()->addAction( action );
  if ( ( mSendQueued = mainWidget->action("send_queued") ) )
    contextMenu()->addAction( mSendQueued );
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

  kDebug() << "Setting systray mMode to" << newMode;
  mMode = newMode;

  switch ( mMode ) {
  case GlobalSettings::EnumSystemTrayPolicy::ShowAlways:
    setStatus( KStatusNotifierItem::Active );
    break;
  case GlobalSettings::EnumSystemTrayPolicy::ShowOnUnread:
    setStatus( mCount > 0 ? KStatusNotifierItem::Active : KStatusNotifierItem::Passive );
    break;
  default:
    kDebug() << "Unknown systray mode" << mMode;
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
    int overlaySize = KIconLoader::SizeSmallMedium;

    QString countString = QString::number( mCount );
    QFont countFont = KGlobalSettings::generalFont();
    countFont.setBold(true);

    // decrease the size of the font for the number of unread messages if the
    // number doesn't fit into the available space
    float countFontSize = countFont.pointSizeF();
    QFontMetrics qfm( countFont );
    int width = qfm.width( countString );
    if( width > (overlaySize - 2) )
    {
      countFontSize *= float( overlaySize - 2 ) / float( width );
      countFont.setPointSizeF( countFontSize );
    }

    // Paint the number in a pixmap
    QPixmap overlayImage( overlaySize, overlaySize );
    overlayImage.fill( Qt::transparent );

    QPainter p( &overlayImage );
    p.setFont( countFont );
    KColorScheme scheme( QPalette::Active, KColorScheme::View );

    qfm = QFontMetrics( countFont );
    QRect boundingRect = qfm.tightBoundingRect( countString );
    boundingRect.adjust( 0, 0, 0, 2 );
    boundingRect.setHeight( qMin( boundingRect.height(), overlaySize ) );
    boundingRect.moveTo( (overlaySize - boundingRect.width()) / 2,
                         ((overlaySize - boundingRect.height()) / 2) - 1 );
    p.setOpacity( 0.7 );
    p.setBrush( scheme.background( KColorScheme::LinkBackground ) );
    p.setPen( scheme.background( KColorScheme::LinkBackground ).color() );
    p.drawRoundedRect( boundingRect, 2.0, 2.0 );

    p.setBrush( Qt::NoBrush );
    p.setPen( scheme.foreground( KColorScheme::LinkText ).color() );
    p.setOpacity( 1.0 );
    p.drawText( overlayImage.rect(), Qt::AlignCenter, countString );
    p.end();

    setOverlayIconByPixmap( overlayImage );
  } else
  {
    setOverlayIconByPixmap( QPixmap() );
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
#if 0
  mFoldersWithUnread.clear();
  mPendingUpdates.clear();
#endif
  mCount = 0;

  if ( mMode == GlobalSettings::EnumSystemTrayPolicy::ShowOnUnread ) {
    setStatus( KStatusNotifierItem::Passive );
  }
#if 0
  /** Disconnect all previous connections */
  disconnect(this, SLOT(updateNewMessageNotification(KMFolder *)));

  QStringList folderNames;
  QList<QPointer<KMFolder> > folderList;
  kmkernel->folderMgr()->createFolderList(&folderNames, &folderList);
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
#if 0
  QStringList::iterator strIt = folderNames.begin();

  for(QList<QPointer<KMFolder> >::iterator it = folderList.begin();
     it != folderList.end() && strIt != folderNames.end(); ++it, ++strIt)
  {
    KMFolder * currentFolder = *it;
    QString currentName = *strIt;
#if 0
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
      disconnect( currentFolder, SIGNAL( numUnreadMsgsChanged(KMFolder*) ),
                  this, SLOT( updateNewMessageNotification(KMFolder *) ) );
    }
#endif
  }
#endif
}

/**
 * On left mouse click, switch focus to the first KMMainWidget.  On right
 * click, bring up a list of all folders with a count of unread messages.
 */
void KMSystemTray::slotActivated()
{
  if( mParentVisible && mainWindowIsOnCurrentDesktop() )
    hideKMail();
  else
    showKMail();
}

void KMSystemTray::slotContextMenuAboutToShow()
{
  // Rebuild popup menu before show to minimize race condition if
  // the base KMainWidget is closed.
  buildPopupMenu();

  if ( mNewMessagesPopup != 0 ) {
    contextMenu()->removeAction( mNewMessagesPopup->menuAction() );
    delete mNewMessagesPopup;
    mNewMessagesPopup = 0;
  }
#if 0
  if ( mFoldersWithUnread.count() > 0 )
  {
    mNewMessagesPopup = new KMenu();

    QMap<QPointer<KMFolder>, int>::Iterator it = mFoldersWithUnread.begin();
    QSignalMapper *folderMapper = new QSignalMapper( this );
    connect( folderMapper, SIGNAL( mapped( int ) ),
             this, SLOT( selectedAccount( int ) ) );

    for(uint i=0; it != mFoldersWithUnread.end(); ++i)
    {
      //TODO port it
      //mPopupFolders.append( it.key() );
      QString folderText = prettyName(it.key()) + " (" + QString::number(it.value()) + ')';
      QAction *action = new QAction( folderText, this );
      connect( action, SIGNAL( triggered( bool ) ),
               folderMapper, SLOT( map() ) );
      folderMapper->setMapping( action, i );
      mNewMessagesPopup->addAction( action );
      ++it;
    }

    mNewMessagesPopup->setTitle( i18n("New Messages In") );
    contextMenu()->insertAction( mSendQueued, mNewMessagesPopup->menuAction() );

    kDebug() << "Folders added";
  }
#endif
}

/**
 * Return the name of the folder in which the mail is deposited, prepended
 * with the account name if the folder is IMAP.
 */
#if 0
QString KMSystemTray::prettyName(KMFolder * fldr)
{

  QString rvalue = fldr->label();
#if 0
  if(fldr->folderType() == KMFolderTypeImap)
  {
#if 0 //TODO port to akonadi
    KMFolderImap * imap = dynamic_cast<KMFolderImap*> (fldr->storage());
    assert(imap);

    if((imap->account() != 0) &&
       (imap->account()->name() != 0) )
    {
      kDebug() << "IMAP folder, prepend label with type";
      rvalue = imap->account()->name() + "->" + rvalue;
    }
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  }

  kDebug() << "Got label" << rvalue;
#endif
  return rvalue;
}
#endif

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
#if 0
void KMSystemTray::updateNewMessageNotification(KMFolder * fldr)
{
#if 0
  //We don't want to count messages from search folders as they
  //  already counted as part of their original folders
  if( !fldr ||
      fldr->folderType() == KMFolderTypeSearch )
  {
    // kDebug() << "Null or a search folder, can't mess with that";
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
#endif
}
#endif

void KMSystemTray::updateNewMessages()
{
#if 0
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
      //kDebug() << "There are now" << mFoldersWithUnread.count() << "folders with unread";
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
      if ( mMode == GlobalSettings::EnumSystemTrayPolicy::ShowOnUnread ) {
        setStatus( KStatusNotifierItem::Active );
      }
    }
    else {
      if ( unread == 0 ) {
        //kDebug() << "Removing folder from internal store" << fldr->name();

        // Remove the folder from the internal store
        mFoldersWithUnread.remove(fldr);

        // if this was the last folder in the dictionary, hide the systemtray icon
        if ( mFoldersWithUnread.count() == 0 ) {
          mPopupFolders.clear();
          disconnect ( this, SLOT( selectedAccount( int ) ) );
          mCount = 0;

          if ( mMode == GlobalSettings::EnumSystemTrayPolicy::ShowOnUnread ) {
            setStatus( KStatusNotifierItem::Passive );
          }
        }
      }
    }
  }
  mPendingUpdates.clear();
  updateCount();

  // Update tooltip to reflect count of unread messages
  setToolTipSubTitle( mCount == 0 ? i18n("There are no unread messages")
                                  : i18np("1 unread message",
                                          "%1 unread messages",
                                          mCount));

  mLastUpdate = time( 0 );
#endif
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
  Akonadi::Collection fldr = mPopupFolders.at(id);
  if(!fldr.isValid()) return;
  mainWidget->selectCollectionFolder( fldr );
}

bool KMSystemTray::hasUnreadMail() const
{
  return ( mCount != 0 );
}

#include "kmsystemtray.moc"
