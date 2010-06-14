// -*- mode: C++; c-file-style: "gnu" -*-
/***************************************************************************
                          kmsystemtray.cpp  -  description
                             ------------------
    begin                : Fri Aug 31 22:38:44 EDT 2001
    copyright            : (C) 2001 by Ryan Breen
    email                : ryan@porivo.com

    Copyright (c) 2010 Montel Laurent <montel@kde.org>
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
#include "foldercollection.h"
#include "globalsettings.h"



#include <kxmlguiwindow.h>
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <kcolorscheme.h>
#include <kwindowsystem.h>
#include <kdebug.h>
#include <KMenu>

#include <QPainter>
#include <QWidget>
#include <QObject>
#include <QSignalMapper>

#include <Akonadi/ChangeRecorder>
#include <Akonadi/EntityTreeModel>
#include <Akonadi/CollectionModel>
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


  connect( this, SIGNAL( activateRequested( bool, const QPoint& ) ),
           this, SLOT( slotActivated() ) );
  connect( contextMenu(), SIGNAL( aboutToShow() ),
           this, SLOT( slotContextMenuAboutToShow() ) );

  connect( kmkernel->monitor(), SIGNAL( collectionStatisticsChanged( Akonadi::Collection::Id, const Akonadi::CollectionStatistics &) ), SLOT( slotCollectionChanged( const Akonadi::Collection::Id, const Akonadi::CollectionStatistics& ) ) );

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
  mNewMessagesPopup = new KMenu();
  fillFoldersMenu( mNewMessagesPopup, KMKernel::self()->entityTreeModel() );
  connect( mNewMessagesPopup, SIGNAL( triggered(QAction*) ), this,
           SLOT( slotSelectCollection(QAction*) ) );


  if ( mCount > 0 ) {
    mNewMessagesPopup->setTitle( i18n("New Messages In") );
    contextMenu()->insertAction( mSendQueued, mNewMessagesPopup->menuAction() );
  }
}

void KMSystemTray::fillFoldersMenu( QMenu *menu, const QAbstractItemModel *model, const QString& parentName, const QModelIndex& parentIndex )
{
  const int rowCount = model->rowCount( parentIndex );
  for ( int row = 0; row < rowCount; ++row ) {
    const QModelIndex index = model->index( row, 0, parentIndex );
    const Akonadi::Collection collection = model->data( index, Akonadi::CollectionModel::CollectionRole ).value<Akonadi::Collection>();
    if ( collection.resource() == QLatin1String( "akonadi_nepomuktag_resource" )
         || collection.resource() == QLatin1String( "akonadi_search_resource" ) )
      continue;
    Akonadi::CollectionStatistics statistics = collection.statistics();
    qint64 count = qMax( 0LL, statistics.unreadCount() );
    if ( count >= 0 ) {
      QSharedPointer<FolderCollection> col = FolderCollection::forCollection( collection );
      if ( col && col->ignoreNewMail() )
        continue;
    }
    mCount += count;

    QString label = parentName.isEmpty() ? "" : parentName + "->";
    label += model->data( index ).toString();
    label.replace( QLatin1String( "&" ), QLatin1String( "&&" ) );

    if ( model->rowCount( index ) > 0 ) {
      // new level
      if ( count > 0 ) {
        QAction * action = menu->addAction( label );
        action->setData( collection.id() );
      }
      fillFoldersMenu( menu, model, label, index );

    } else {
      if ( count > 0 ) {
        // insert an item
        QAction* action = menu->addAction( label );
        action->setData( collection.id() );
      }
    }
  }
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

void KMSystemTray::initListOfCollection()
{
  mCount = 0;
  if ( mMode == GlobalSettings::EnumSystemTrayPolicy::ShowOnUnread ) {
    setStatus( KStatusNotifierItem::Passive );
  }

  unreadMail( KMKernel::self()->entityTreeModel() );
}

void KMSystemTray::unreadMail( const QAbstractItemModel *model, const QModelIndex& parentIndex  )
{
  const int rowCount = model->rowCount( parentIndex );
  for ( int row = 0; row < rowCount; ++row ) {
    const QModelIndex index = model->index( row, 0, parentIndex );
    const Akonadi::Collection collection = model->data( index, Akonadi::CollectionModel::CollectionRole ).value<Akonadi::Collection>();

    if ( collection.resource() == QLatin1String( "akonadi_nepomuktag_resource" )
         || collection.resource() == QLatin1String( "akonadi_search_resource" ) )
      continue;

    Akonadi::CollectionStatistics statistics = collection.statistics();
    qint64 count = qMax( 0LL, statistics.unreadCount() );

    if ( count >= 0 ) {
      QSharedPointer<FolderCollection> col = FolderCollection::forCollection( collection );
      if ( col && col->ignoreNewMail() )
        continue;
    }

    mCount += count;
    if ( model->rowCount( index ) > 0 ) {
      unreadMail( model, index );
    }
  }
  // Update tooltip to reflect count of unread messages
  setToolTipSubTitle( mCount == 0 ? i18n("There are no unread messages")
                                  : i18np("1 unread message",
                                          "%1 unread messages",
                                          mCount));
  // Make sure the icon will be displayed
  if ( ( mMode == GlobalSettings::EnumSystemTrayPolicy::ShowOnUnread ) &&
       status() == KStatusNotifierItem::Passive && mCount > 0) {
    setStatus( KStatusNotifierItem::Active );
  }

  kDebug()<<" mCount :"<<mCount;
  updateCount();
}

void KMSystemTray::slotCollectionChanged( const Akonadi::Collection::Id, const Akonadi::CollectionStatistics& )
{
  mCount = 0;
  initListOfCollection();
}

bool KMSystemTray::hasUnreadMail() const
{
  return ( mCount != 0 );
}

void KMSystemTray::slotSelectCollection(QAction*act)
{
  const Akonadi::Collection::Id id = act->data().value<Akonadi::Collection::Id>();
  KMKernel::self()->selectCollectionFromId( id );
}

void KMSystemTray::updateSystemTray()
{
  mCount = 0;
  initListOfCollection();
}

#include "kmsystemtray.moc"
