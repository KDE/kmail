/* -*- mode: C++; c-file-style: "gnu" -*-
  This file is part of KMail, the KDE mail client.
  Copyright (c) 2009 Montel Laurent <montel@kde.org>

  KMail is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  KMail is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "foldertreeview.h"
#include <kdebug.h>
#include <KLocale>
#include <akonadi/entitytreemodel.h>
#include <akonadi/collectionstatistics.h>
#include <akonadi/collectionstatisticsdelegate.h>
#include "kmkernel.h"
#include <KMessageBox>
#include <KGuiItem>
#include <KMenu>

FolderTreeView::FolderTreeView(QWidget *parent, bool showUnreadCount )
  : Akonadi::EntityTreeView( parent ), mbDisableContextMenuAndExtraColumn( false )
{
  init(showUnreadCount);
}


FolderTreeView::FolderTreeView(KXMLGUIClient *xmlGuiClient, QWidget *parent, bool showUnreadCount )
  :Akonadi::EntityTreeView( xmlGuiClient, parent ), mbDisableContextMenuAndExtraColumn( false )
{
  init(showUnreadCount);
}


FolderTreeView::~FolderTreeView()
{
}


void FolderTreeView::setTooltipsPolicy( FolderTreeWidget::ToolTipDisplayPolicy policy )
{
  mToolTipDisplayPolicy = policy;
  writeConfig();
}

void FolderTreeView::disableContextMenuAndExtraColumn()
{
  mbDisableContextMenuAndExtraColumn = true;
  const int nbColumn = header()->count();
  for ( int i = 1; i <nbColumn; ++i )
  {
    setColumnHidden( i, true );
  }
}

void FolderTreeView::init( bool showUnreadCount )
{
  setIconSize( QSize( 22, 22 ) );
  mSortingPolicy = FolderTreeWidget::SortByCurrentColumn;

  header()->setContextMenuPolicy( Qt::CustomContextMenu );
  connect( header(), SIGNAL( customContextMenuRequested( const QPoint& ) ),
           SLOT( slotHeaderContextMenuRequested( const QPoint& ) ) );
  readConfig();

  mCollectionStatisticsDelegate = new Akonadi::CollectionStatisticsDelegate(this);
  setItemDelegate(mCollectionStatisticsDelegate);
  mCollectionStatisticsDelegate->setUnreadCountShown( showUnreadCount && !header()->isSectionHidden( 1 ) );
}

void FolderTreeView::writeConfig()
{
  KConfigGroup myGroup( KMKernel::config(), "MainFolderView");
  myGroup.writeEntry( "IconSize", iconSize().width() );
  myGroup.writeEntry( "ToolTipDisplayPolicy", ( int ) mToolTipDisplayPolicy );
  myGroup.writeEntry( "SortingPolicy", ( int ) mSortingPolicy );
}

void FolderTreeView::readConfig()
{
  KConfigGroup myGroup( KMKernel::config(), "MainFolderView" );
  int iIconSize = myGroup.readEntry( "IconSize", iconSize().width() );
  if ( iIconSize < 16 || iIconSize > 32 )
    iIconSize = 22;
  setIconSize( QSize( iIconSize, iIconSize ) );
  setSortingPolicy( ( FolderTreeWidget::SortingPolicy )myGroup.readEntry( "SortingPolicy", ( int ) mSortingPolicy ) );
}

void FolderTreeView::slotHeaderContextMenuRequested( const QPoint&pnt )
{
  if ( mbDisableContextMenuAndExtraColumn ) {
    readConfig();
    return;
  }

  // the menu for the columns
  KMenu menu;
  QAction *act;
  menu.addTitle( i18n("View Columns") );
  const int nbColumn = header()->count();
  for ( int i = 1; i <nbColumn; ++i )
  {
    act = menu.addAction( model()->headerData( i, Qt::Horizontal ).toString() );
    act->setCheckable( true );
    act->setChecked( !header()->isSectionHidden( i ) );
    act->setData( QVariant( i ) );
    if ( i == 0)
       act->setEnabled( false );
    connect( act,  SIGNAL( triggered( bool ) ),
             SLOT( slotHeaderContextMenuChangeHeader( bool ) ) );
  }


  menu.addTitle( i18n( "Icon Size" ) );

  static int icon_sizes[] = { 16, 22, 32 /*, 48, 64, 128 */ };

  QActionGroup *grp = new QActionGroup( &menu );

  for ( int i = 0; i < (int)( sizeof( icon_sizes ) / sizeof( int ) ); i++ )
  {
    act = menu.addAction( QString("%1x%2").arg( icon_sizes[ i ] ).arg( icon_sizes[ i ] ) );
    act->setCheckable( true );
    grp->addAction( act );
    if ( iconSize().width() == icon_sizes[ i ] )
      act->setChecked( true );
    act->setData( QVariant( icon_sizes[ i ] ) );

    connect( act, SIGNAL( triggered( bool ) ),
             SLOT( slotHeaderContextMenuChangeIconSize( bool ) ) );
  }
  menu.addTitle( i18n( "Display Tooltips" ) );

  grp = new QActionGroup( &menu );

  act = menu.addAction( i18nc("@action:inmenu Always display tooltips", "Always") );
  act->setCheckable( true );
  grp->addAction( act );
  act->setChecked( mToolTipDisplayPolicy == FolderTreeWidget::DisplayAlways );
  act->setData( QVariant( (int)FolderTreeWidget::DisplayAlways ) );
  connect( act, SIGNAL( triggered( bool ) ),
           SLOT( slotHeaderContextMenuChangeToolTipDisplayPolicy( bool ) ) );
  act = menu.addAction( i18nc("@action:inmenu", "When Text Obscured") );
  act->setCheckable( true );

  //Port it !!!!
  act->setEnabled( false );
  grp->addAction( act );
  act->setChecked( mToolTipDisplayPolicy == FolderTreeWidget::DisplayWhenTextElided );
  act->setData( QVariant( (int)FolderTreeWidget::DisplayWhenTextElided ) );
  connect( act, SIGNAL( triggered( bool ) ),
           SLOT( slotHeaderContextMenuChangeToolTipDisplayPolicy( bool ) ) );

  act = menu.addAction( i18nc("@action:inmenu Never display tooltips.", "Never") );
  act->setCheckable( true );
  grp->addAction( act );
  act->setChecked( mToolTipDisplayPolicy == FolderTreeWidget::DisplayNever );
  act->setData( QVariant( (int)FolderTreeWidget::DisplayNever ) );
  connect( act, SIGNAL( triggered( bool ) ),
           SLOT( slotHeaderContextMenuChangeToolTipDisplayPolicy( bool ) ) );

  menu.addTitle( i18nc("@action:inmenu", "Sort Items" ) );

  grp = new QActionGroup( &menu );

  act = menu.addAction( i18nc("@action:inmenu", "Automatically, by Current Column") );
  act->setCheckable( true );
  grp->addAction( act );
  act->setChecked( mSortingPolicy == FolderTreeWidget::SortByCurrentColumn );
  act->setData( QVariant( (int)FolderTreeWidget::SortByCurrentColumn ) );
  connect( act, SIGNAL( triggered( bool ) ),
           SLOT( slotHeaderContextMenuChangeSortingPolicy( bool ) ) );

  act = menu.addAction( i18nc("@action:inmenu", "Manually, by Drag And Drop") );
  act->setCheckable( true );
  grp->addAction( act );
  act->setChecked( mSortingPolicy == FolderTreeWidget::SortByDragAndDropKey );
  act->setData( QVariant( (int)FolderTreeWidget::SortByDragAndDropKey ) );
  connect( act, SIGNAL( triggered( bool ) ),
           SLOT( slotHeaderContextMenuChangeSortingPolicy( bool ) ) );

  menu.exec( header()->mapToGlobal( pnt ) );
}

void FolderTreeView::slotHeaderContextMenuChangeSortingPolicy( bool )
{
  QAction *act = dynamic_cast< QAction * >( sender() );
  if ( !act )
    return;

  QVariant data = act->data();

  bool ok;
  int policy = data.toInt( &ok );
  if ( !ok )
    return;

  setSortingPolicy( ( FolderTreeWidget::SortingPolicy )policy );
}


void FolderTreeView::setSortingPolicy( FolderTreeWidget::SortingPolicy policy )
{
  mSortingPolicy = policy;
  switch ( mSortingPolicy )
  {
  case FolderTreeWidget::SortByCurrentColumn:
      header()->setClickable( true );
      header()->setSortIndicatorShown( true );
      setSortingEnabled( true );
      emit manualSortingChanged( false );
    break;
  case FolderTreeWidget::SortByDragAndDropKey:
      header()->setClickable( false );
      header()->setSortIndicatorShown( false );
#if 0
      //
      // Qt 4.5 introduced a nasty bug here:
      // Sorting must be enabled in order to sortByColumn() to work.
      // If sorting is disabled it disconnects some internal signal/slot pairs
      // and calling sortByColumn() silently has no effect.
      // This is a bug as we actually DON'T want automatic sorting to be
      // performed by the view whenever it wants. We want to control sorting.
      //
      setSortingEnabled( true ); // hack for qutie bug: the param here should be false
      sortByColumn( 0, Qt::AscendingOrder );
#endif
      setSortingEnabled( false ); // hack for qutie bug: this call shouldn't be here at all
      emit manualSortingChanged( true );

    break;
    default:
      // should never happen
    break;
  }
  writeConfig();
}

void FolderTreeView::slotHeaderContextMenuChangeToolTipDisplayPolicy( bool )
{
  QAction *act = dynamic_cast< QAction * >( sender() );
  if ( !act )
    return;

  QVariant data = act->data();

  bool ok;
  const int id = data.toInt( &ok );
  if ( !ok )
    return;
  emit changeTooltipsPolicy( ( FolderTreeWidget::ToolTipDisplayPolicy )id );
}

void FolderTreeView::slotHeaderContextMenuChangeHeader( bool )
{
  QAction *act = dynamic_cast< QAction * >( sender() );
  if ( !act )
    return;

  QVariant data = act->data();

  bool ok;
  const int id = data.toInt( &ok );
  if ( !ok )
    return;

  if ( id > header()->count() )
    return;

  if ( id == 1 )
    mCollectionStatisticsDelegate->setUnreadCountShown(!act->isChecked());

  setColumnHidden( id, !act->isChecked() );
}

void FolderTreeView::slotHeaderContextMenuChangeIconSize( bool )
{
  QAction *act = dynamic_cast< QAction * >( sender() );
  if ( !act )
    return;

  QVariant data = act->data();

  bool ok;
  const int size = data.toInt( &ok );
  if ( !ok )
    return;

  setIconSize( QSize( size, size ) );
  writeConfig();
}

void FolderTreeView::selectModelIndex( const QModelIndex & index )
{
  if ( index.isValid() ) {
    clearSelection();
    scrollTo( index );
    selectionModel()->setCurrentIndex( index, QItemSelectionModel::NoUpdate );
  }
}

void FolderTreeView::slotSelectFocusFolder()
{
  const QModelIndex index = currentIndex();
  if( index.isValid() )
    setCurrentIndex( index );
}

void FolderTreeView::slotFocusNextFolder()
{
  const QModelIndex nextFolder = selectNextFolder( currentIndex() );

  if ( nextFolder.isValid() ) {
    expand( nextFolder );
    selectModelIndex( nextFolder );
  }
}

QModelIndex FolderTreeView::selectNextFolder( const QModelIndex & current )
{
  QModelIndex below;
  if ( current.isValid() ) {
    model()->fetchMore( current );
    if ( model()->hasChildren( current ) ) {
      expand( current );
      below = indexBelow( current );
    } else if ( current.row() < model()->rowCount( model()->parent( current ) ) -1 ) {
      below = model()->index( current.row()+1, current.column(), model()->parent( current ) );
    } else {
      below = indexBelow( current );
    }
  }
  return below;
}

void FolderTreeView::slotFocusPrevFolder()
{
  const QModelIndex current = currentIndex();
  if ( current.isValid() ) {
    QModelIndex above = indexAbove( current );
    selectModelIndex( above );
  }
}

void FolderTreeView::selectNextUnreadFolder( bool confirm )
{
  QModelIndex current = selectNextFolder( currentIndex() );
  while ( current.isValid() ) {
    QModelIndex nextIndex;
    if ( isUnreadFolder( current,nextIndex,FolderTreeView::Next,confirm ) ) {
      return;
    }
    current = nextIndex;
  }
  //Move to top
  current = model()->index( 0, 0 );
  while ( current.isValid() ) {
    QModelIndex nextIndex;
    if ( isUnreadFolder( current,nextIndex, FolderTreeView::Next,confirm ) ) {
      return;
    }
    current = nextIndex;
  }
}

bool FolderTreeView::isUnreadFolder( const QModelIndex & current, QModelIndex &index, FolderTreeView::Move move, bool confirm )
{
  if ( current.isValid() ) {

    if ( move == FolderTreeView::Next )
      index = selectNextFolder( current );
    else if ( move == FolderTreeView::Previous )
      index = indexAbove( current );

    if ( index.isValid() ) {
      Akonadi::Collection collection = index.model()->data( current, Akonadi::EntityTreeModel::CollectionRole ).value<Akonadi::Collection>();
      if ( collection.isValid() ) {
        if ( collection.statistics().unreadCount()>0 ) {
          if ( !confirm ) {
            selectModelIndex( current );
            return true;
          } else {
            // Skip drafts, sent mail and templates as well, when reading mail with the
            // space bar - but not when changing into the next folder with unread mail
            // via ctrl+ or ctrl- so we do this only if (confirm == true), which means
            // we are doing readOn.

            if ( collection == KMKernel::self()->draftsCollectionFolder() ||
                 collection == KMKernel::self()->templatesCollectionFolder() ||
                 collection == KMKernel::self()->sentCollectionFolder() )
              return false;

            // warn user that going to next folder - but keep track of
            // whether he wishes to be notified again in "AskNextFolder"
            // parameter (kept in the config file for kmail)
            if (KMessageBox::questionYesNo(
                                           this,
                                           i18n( "<qt>Go to the next unread message in folder <b>%1</b>?</qt>" , collection.name() ),
                                           i18n( "Go to Next Unread Message" ),
                                           KGuiItem( i18n( "Go To" ) ),
                                           KGuiItem( i18n( "Do Not Go To" ) ), // defaults
                                           ":kmail_AskNextFolder",
                                           false
                                           ) == KMessageBox::No
                )
              return true; // assume selected (do not continue looping)

            selectModelIndex( current );
            return true;
          }
        }
      }
    }
  }
  return false;
}

void FolderTreeView::selectPrevUnreadFolder( bool confirm )
{
  kDebug()<<" Need to implement FolderTreeView::selectPrevUnreadFolder()";

  QModelIndex current = indexAbove( currentIndex() );
  while ( current.isValid() ) {
    QModelIndex nextIndex;
    if ( isUnreadFolder( current,nextIndex,FolderTreeView::Previous, confirm ) ) {
      return;
    }
    current = nextIndex;
  }
  //TODO start at the end.
}

Akonadi::Collection FolderTreeView::currentFolder() const
{
  const QModelIndex current = currentIndex();
  if ( current.isValid() ) {
    const Akonadi::Collection collection = current.model()->data( current, Akonadi::EntityTreeModel::CollectionRole ).value<Akonadi::Collection>();
    return collection;
  }
  return Akonadi::Collection();
}

#include "foldertreeview.moc"
