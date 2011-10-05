// -*- mode: C++; c-file-style: "gnu" -*-
/***************************************************************************
                          kmsystemtray.cpp  -  description
                             ------------------
    begin                : Fri Aug 31 22:38:44 EDT 2001
    copyright            : (C) 2001 by Ryan Breen
    email                : ryan@porivo.com

    Copyright (c) 2010, 2011 Montel Laurent <montel@kde.org>
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
#include "mailutil.h"
#include "mailcommon/mailkernel.h"
#include "mailcommon/foldertreeview.h"

#include <QItemSelectionModel>

#include <kiconloader.h>
#include <kcolorscheme.h>
#include <kwindowsystem.h>
#include <kdebug.h>
#include <KMenu>
#include <KLocale>

#include <QPainter>
#include <QWidget>
#include <QObject>

#include <Akonadi/ChangeRecorder>
#include <Akonadi/EntityTreeModel>
#include <Akonadi/CollectionModel>
#include <assert.h>

using namespace MailCommon;

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
    mDesktopOfMainWin( 0 ),
    mMode( GlobalSettings::EnumSystemTrayPolicy::ShowOnUnread ),
    mCount( 0 ),
    mNewMessagesPopup( 0 ),
    mSendQueued( 0 )
{
  kDebug() << "Initting systray";
  setToolTipTitle( i18n("KMail") );
  setToolTipIconByName( "kmail" );
  setIconByName( "kmail" );
  mIcon = KIcon( "kmail" );

#ifdef Q_WS_X11
  KMMainWidget * mainWidget = kmkernel->getKMMainWidget();
  if ( mainWidget ) {
    QWidget * mainWin = mainWidget->window();
    if ( mainWin ) {
      mDesktopOfMainWin = KWindowSystem::windowInfo( mainWin->winId(),
                                            NET::WMDesktop ).desktop();
    }
  }
#endif
  // register the applet with the kernel
  kmkernel->registerSystemTrayApplet( this );


  connect( this, SIGNAL(activateRequested(bool,QPoint)),
           this, SLOT(slotActivated()) );
  connect( contextMenu(), SIGNAL(aboutToShow()),
           this, SLOT(slotContextMenuAboutToShow()) );

  connect( kmkernel->folderCollectionMonitor(), SIGNAL(collectionStatisticsChanged(Akonadi::Collection::Id,Akonadi::CollectionStatistics)), SLOT(slotCollectionStatisticsChanged(Akonadi::Collection::Id,Akonadi::CollectionStatistics)) );

  connect( kmkernel->folderCollectionMonitor(), SIGNAL(collectionAdded(Akonadi::Collection,Akonadi::Collection)), this, SLOT(initListOfCollection()) );
  connect( kmkernel->folderCollectionMonitor(), SIGNAL(collectionRemoved(Akonadi::Collection)), this, SLOT(initListOfCollection()) );
  connect( kmkernel->folderCollectionMonitor(), SIGNAL(collectionSubscribed(Akonadi::Collection,Akonadi::Collection)),SLOT(initListOfCollection()) );
  connect( kmkernel->folderCollectionMonitor(), SIGNAL(collectionUnsubscribed(Akonadi::Collection)),SLOT(initListOfCollection()) );

  
  initListOfCollection();

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

  contextMenu()->addTitle(qApp->windowIcon(), i18n("KMail"));
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

  if ( ( action = actionCollection()->action("file_quit") ) )
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
  if(mCount == 0)
  {
    setIconByName( "kmail" );
    return;
  }
  const int overlaySize = KIconLoader::SizeSmallMedium;

  const QString countString = QString::number( mCount );
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
  QPixmap overlayPixmap( overlaySize, overlaySize );
  overlayPixmap.fill( Qt::transparent );

  QPainter p( &overlayPixmap );
  p.setFont( countFont );
  KColorScheme scheme( QPalette::Active, KColorScheme::View );

  p.setBrush( Qt::NoBrush );
  p.setPen( scheme.foreground( KColorScheme::LinkText ).color() );
  p.setOpacity( 1.0 );
  p.drawText( overlayPixmap.rect(), Qt::AlignCenter, countString );
  p.end();

  QPixmap iconPixmap = mIcon.pixmap(overlaySize, overlaySize);

  QPainter pp(&iconPixmap);
  pp.drawPixmap(0, 0, overlayPixmap);
  pp.end();

  setIconByPixmap( iconPixmap );
}


/**
 * On left mouse click, switch focus to the first KMMainWidget.  On right
 * click, bring up a list of all folders with a count of unread messages.
 */
void KMSystemTray::slotActivated()
{
  KMMainWidget * mainWidget = kmkernel->getKMMainWidget();
  if ( !mainWidget )
    return ;

  QWidget *mainWin = kmkernel->getKMMainWidget()->window();
  if ( !mainWin )
    return ;

  KWindowInfo cur = KWindowSystem::windowInfo( mainWin->winId(), NET::WMDesktop );
  
  int currentDesktop = KWindowSystem::currentDesktop();
  bool wasMinimized = cur.isMinimized();
  
  if ( cur.valid() )
    mDesktopOfMainWin = cur.desktop();

  if (wasMinimized  && (currentDesktop != mDesktopOfMainWin) && ( mDesktopOfMainWin == NET::OnAllDesktops ))
    KWindowSystem::setOnDesktop(mainWin->winId(), currentDesktop);

  if ( mDesktopOfMainWin == NET::OnAllDesktops )
    KWindowSystem::setOnAllDesktops( mainWin->winId(), true );
  
  KWindowSystem::activateWindow( mainWin->winId() );
  
  if (wasMinimized )
    kmkernel->raise();
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
  fillFoldersMenu( mNewMessagesPopup, KMKernel::self()->treeviewModelSelection() );
  
  connect( mNewMessagesPopup, SIGNAL(triggered(QAction*)), this,
           SLOT(slotSelectCollection(QAction*)) );


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
    if ( excludeFolder( collection ) )
      continue;
    Akonadi::CollectionStatistics statistics = collection.statistics();
    const qint64 count = qMax( 0LL, statistics.unreadCount() );
    if ( count > 0 ) {
      const QSharedPointer<FolderCollection> col = FolderCollection::forCollection( collection, false );
      if ( col && col->ignoreNewMail() )
        continue;
    }
    mCount += count;

    QString label = parentName.isEmpty() ? QLatin1String("") : QString(parentName + QLatin1String("->"));
    label += model->data( index ).toString();
    label.replace( QLatin1String( "&" ), QLatin1String( "&&" ) );
    if ( count > 0 ) {
      // insert an item
      QAction* action = menu->addAction( label );
      action->setData( collection.id() );
    }
    if ( model->rowCount( index ) > 0 ) {
      fillFoldersMenu( menu, model, label, index );
    }
  }
}



void KMSystemTray::hideKMail()
{
  if (!kmkernel->getKMMainWidget())
    return;
  QWidget *mainWin = kmkernel->getKMMainWidget()->window();
  assert(mainWin);
  if(mainWin)
  {
#ifdef Q_WS_X11
    mDesktopOfMainWin = KWindowSystem::windowInfo( mainWin->winId(),
                                          NET::WMDesktop ).desktop();
    // iconifying is unnecessary, but it looks cooler
    KWindowSystem::minimizeWindow( mainWin->winId() );
#endif
    mainWin->hide();
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

    if ( excludeFolder( collection ) )
      continue;

    const Akonadi::CollectionStatistics statistics = collection.statistics();
    const qint64 count = qMax( 0LL, statistics.unreadCount() );

    if ( count > 0 ) {
      const QSharedPointer<FolderCollection> col = FolderCollection::forCollection( collection, false );
      if ( col && !col->ignoreNewMail() ) {
        mCount += count;
      }
    }

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

  //kDebug()<<" mCount :"<<mCount;
  updateCount();
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
  initListOfCollection();
}

void KMSystemTray::slotCollectionStatisticsChanged( Akonadi::Collection::Id id,const Akonadi::CollectionStatistics& )
{
  //Exclude sent mail folder

  if ( CommonKernel->outboxCollectionFolder().id() == id ||
       CommonKernel->sentCollectionFolder().id() == id ||
       CommonKernel->templatesCollectionFolder().id() == id ||
       CommonKernel->trashCollectionFolder().id() == id ||
       CommonKernel->draftsCollectionFolder().id() == id ) {
    return;
  }
  initListOfCollection();
}

bool KMSystemTray::excludeFolder( const Akonadi::Collection& collection ) const
{
  if ( CommonKernel->outboxCollectionFolder() == collection ||
       CommonKernel->sentCollectionFolder() == collection ||
       CommonKernel->templatesCollectionFolder() == collection ||
       CommonKernel->trashCollectionFolder() == collection ||
       CommonKernel->draftsCollectionFolder() == collection ) {
    return true;
  }

  if ( MailCommon::Util::isVirtualCollection( collection ) )
    return true;
  
  return false;
}

#include "kmsystemtray.moc"
