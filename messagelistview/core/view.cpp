/******************************************************************************
 *
 *  Copyright 2008 Szymon Tomasz Stefanek <pragma@kvirc.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *******************************************************************************/

#include "messagelistview/core/view.h"

#include "messagelistview/core/aggregation.h"
#include "messagelistview/core/delegate.h"
#include "messagelistview/core/groupheaderitem.h"
#include "messagelistview/core/item.h"
#include "messagelistview/core/manager.h"
#include "messagelistview/core/messageitem.h"
#include "messagelistview/core/model.h"
#include "messagelistview/core/theme.h"
#include "messagelistview/core/storagemodelbase.h"
#include "messagelistview/core/widgetbase.h"

#include <kmime/kmime_dateformatter.h> // kdepimlibs
#include <libkdepim/maillistdrag.h>

#include <QHelpEvent>
#include <QToolTip>
#include <QHeaderView>
#include <QTimer>

#include <KMenu>
#include <KLocale>
#include <KDebug>
#include <KGlobalSettings>

namespace KMail
{

namespace MessageListView
{

namespace Core
{

View::View( Widget *pParent )
  : QTreeView( pParent ), mWidget( pParent )
{
  mTheme = 0;
  mAggregation = 0;
  mDelegate = new Delegate( this );
  mLastCurrentItem = 0;
  mFirstShow = true;

  mSaveThemeColumnStateTimer = new QTimer();
  connect( mSaveThemeColumnStateTimer, SIGNAL( timeout() ), this, SLOT( saveThemeColumnState() ) );

  setItemDelegate( mDelegate );
  setVerticalScrollMode( QAbstractItemView::ScrollPerPixel );
  setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
  setAlternatingRowColors( true );
  setAllColumnsShowFocus( true );
  setSelectionMode( QAbstractItemView::ExtendedSelection );
  viewport()->setAcceptDrops( true );

  //setUniformRowHeights( true );

  header()->setContextMenuPolicy( Qt::CustomContextMenu );
  connect( header(), SIGNAL( customContextMenuRequested( const QPoint& ) ),
           SLOT( slotHeaderContextMenuRequested( const QPoint& ) ) );
  connect( header(), SIGNAL( sectionResized( int, int, int ) ),
           SLOT( slotHeaderSectionResized( int, int ,int ) ) );

  mSaveThemeColumnStateOnSectionResize = true;

  header()->setClickable( true );
  header()->setResizeMode( QHeaderView::Interactive );
  header()->setMinimumSectionSize( 2 ); // QTreeView overrides our sections sizes if we set them smaller than this value
  header()->setDefaultSectionSize( 2 ); // QTreeView overrides our sections sizes if we set them smaller than this value

  mModel = new Model( this );
  setModel( mModel );

  //connect( selectionModel(), SIGNAL( currentChanged( const QModelIndex &, const QModelIndex & ) ),
  //         this, SLOT( slotCurrentIndexChanged( const QModelIndex &, const QModelIndex & ) ) );
  connect( selectionModel(), SIGNAL( selectionChanged( const QItemSelection &, const QItemSelection & ) ),
           this, SLOT( slotSelectionChanged( const QItemSelection &, const QItemSelection & ) ) );


}

View::~View()
{
  if ( mSaveThemeColumnStateTimer->isActive() )
    mSaveThemeColumnStateTimer->stop();
  delete mSaveThemeColumnStateTimer;
}

void View::ignoreCurrentChanges( bool ignore )
{
  if ( ignore )
  {
    disconnect( selectionModel(), SIGNAL( selectionChanged( const QItemSelection &, const QItemSelection & ) ),
                this, SLOT( slotSelectionChanged( const QItemSelection &, const QItemSelection & ) ) );
    viewport()->setUpdatesEnabled( false );
  } else {
    connect( selectionModel(), SIGNAL( selectionChanged( const QItemSelection &, const QItemSelection & ) ),
             this, SLOT( slotSelectionChanged( const QItemSelection &, const QItemSelection & ) ) );
    viewport()->setUpdatesEnabled( true );
  }
}


const StorageModel * View::storageModel() const
{
  return mModel->storageModel();
}

void View::setAggregation( const Aggregation * aggregation )
{
  mAggregation = aggregation;
  mModel->setAggregation( aggregation );

  if ( !mAggregation )
    return;
}

void View::setTheme( Theme * theme )
{
  mNeedToApplyThemeColumns = true;
  mTheme = theme;
  mDelegate->setTheme( theme );
  mModel->setTheme( theme );
}

void View::reload()
{
  setStorageModel( storageModel() );
}

void View::setStorageModel( const StorageModel * storageModel, PreSelectionMode preSelectionMode )
{
  // This will cause the model to be reset.
  mSaveThemeColumnStateOnSectionResize = false;
  mModel->setStorageModel( storageModel, preSelectionMode );
  mSaveThemeColumnStateOnSectionResize = true;
}

void View::modelJobBatchStarted()
{
  // This is called by the model when the first job of a batch starts
  mWidget->viewJobBatchStarted();
}

void View::modelJobBatchTerminated()
{
  // This is called by the model when all the pending jobs have been processed
  mWidget->viewJobBatchTerminated();
}

void View::modelHasBeenReset()
{
  // This is called by Model when it has been reset.
  if ( mNeedToApplyThemeColumns )
    applyThemeColumns();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
// Theme column state machinery
//
// This is yet another beast to beat. The QHeaderView behaviour, at the time of writing,
// is quite unpredictable. This is due to the complex interaction with the model, with the QTreeView
// and due to its attempts to delay the layout jobs. The delayed layouts, especially, may
// cause the widths of the columns to quickly change in an unexpected manner in a place
// where previously they have been always settled to the values you set...
//
// So here we have the tools to:
//
// - Apply the saved state of the theme columns (applyThemeColumns()).
//   This function computes the "best fit" state of the visible columns and tries
//   to apply it to QHeaderView. It also saves the new computed state to the Theme object.
//
// - Explicitly save the column state, used when the user changes the widths or visibility manually.
//   This is called through a delayed timer after a column has been resized or used directly
//   when the visibility state of a column has been changed by toggling a popup menu entry.
//
// - Display the column state context popup menu and handle its actions
//
// - Apply the theme columns when the theme changes, when the model changes or in certain
//   ugly corner cases when the widget is resized or shown.
//
// - Avoid saving a corrupted column state in that QHeaderView can be found *very* frequently.
//

void View::applyThemeColumns()
{
  if ( !mTheme )
    return;

  const QList< Theme::Column * > & columns = mTheme->columns();

  if ( columns.count() < 1 )
    return; // bad theme

  if ( !viewport()->isVisible() )
    return; // invisible

  if ( viewport()->width() < 1 )
    return; // insane width

  // Now we want to distribute the available width on all the visible columns.
  //
  // The rules:
  // - The visible columns will span the width of the view, if possible.
  // - The columns with a saved width should take that width.
  // - The columns on the left should take more space, if possible.
  // - The columns with no text take just slightly more than their size hint.
  //   while the columns with text take possibly a lot more.
  //

  // Note that the first column is always shown (it can't be hidden at all)

  // The algorithm below is a sort of compromise between:
  // - Saving the user preferences for widths
  // - Using exactly the available view space
  //
  // It "tends to work" in all cases:
  // - When there are no user preferences saved and the column widths must be
  //   automatically computed to make best use of available space
  // - When there are user preferences for only some of the columns
  //   and that should be somewhat preserved while still using all the
  //   available space.
  // - When all the columns have well defined saved widths

  QList< Theme::Column * >::ConstIterator it;
  int idx = 0;

  // Gather total size "hint" for visible sections: if the widths of the columns wers
  // all saved then the total hint is equal to the total saved width.

  int totalVisibleWidthHint = 0;
  QList< int > lColumnSizeHints;

  for ( it = columns.begin(); it != columns.end(); ++it )
  {
    if ( ( *it )->currentlyVisible() || ( idx == 0 ) )
    {
      // Column visible
      int savedWidth = ( *it )->currentWidth();
      int hintWidth = mDelegate->sizeHintForItemTypeAndColumn( Item::Message, idx ).width();
      totalVisibleWidthHint += savedWidth > 0 ? savedWidth : hintWidth;
      lColumnSizeHints.append( hintWidth );
    } else {
      // The column is not visible
      lColumnSizeHints.append( -1 ); // dummy
    }
    idx++;
  }

  if ( totalVisibleWidthHint < 16 )
    totalVisibleWidthHint = 16; // be reasonable

  // Now compute somewhat "proportional" widths.
  idx = 0;

  QList< int > lColumnWidths;
  int totalVisibleWidth = 0;

  for ( it = columns.begin(); it != columns.end(); ++it )
  {
    int savedWidth = ( *it )->currentWidth();
    int hintWidth = savedWidth > 0 ? savedWidth : lColumnSizeHints[ idx ];
    int realWidth;

    if ( ( *it )->currentlyVisible() || ( idx == 0 ) )
    {
      if ( ( *it )->containsTextItems() )
      {
         // the column contains text items, it should get more space (if possible)
         realWidth = ( ( hintWidth * viewport()->width() ) / totalVisibleWidthHint );
      } else {
         // the column contains no text items, it should get exactly its hint/saved width.
         realWidth = hintWidth;
      }

      if ( realWidth < 2 )
        realWidth = 2; // don't allow very insane values 

      // kDebug() << "Column " << idx << " saved " << savedWidth << " hint " << hintWidth << " chosen " << realWidth;
      totalVisibleWidth += realWidth;
    } else {
      // Column not visible
      realWidth = -1;
    }

    lColumnWidths.append( realWidth );

    idx++;
  }

  // kDebug() << "Total visible width before fixing is " << totalVisibleWidth << ", viewport width is " << viewport()->width();

  // Now the algorithm above may be wrong for several reasons...
  // - We're using fixed widths for certain columns and proportional
  //   for others...
  // - The user might have changed the width of the view from the
  //   time in that the widths have been saved
  // - There are some (not well identified) issues with the QTreeView
  //   scrollbar that make our view appear larger or shorter by 2-3 pixels
  //   sometimes.
  // - ...
  // So we correct the previous estimates by trying to use exactly
  // the available space.

  idx = 0;

  if ( totalVisibleWidth != viewport()->width() )
  {
    // The estimated widths were not using exactly the available space.
    if ( totalVisibleWidth < viewport()->width() )
    {
      // We were using less space than available.

      // Give the additional space to the text columns
      // also give more space to the first ones and less space to the last ones
      int available = viewport()->width() - totalVisibleWidth;

      for ( it = columns.begin(); it != columns.end(); ++it )
      {
        if ( ( ( *it )->currentlyVisible() || ( idx == 0 ) ) && ( *it )->containsTextItems() )
        {
          // give more space to this column
          available >>= 1; // eat half of the available space
          lColumnWidths[ idx ] += available; // and give it to this column
          if ( available < 1 )
            break; // no more space to give away
        }

        idx++;
      }

      // if any space is still available, give it to the first column
      if ( available )
        lColumnWidths[ 0 ] += available;
    } else {
      // We were using more space than available

      // If the columns span just a little bit more than the view then
      // try to squeeze them in order to make them fit
      if ( totalVisibleWidth < ( viewport()->width() + 100 ) )
      {
        int missing = totalVisibleWidth - viewport()->width();
        int count = lColumnWidths.count();

        if ( missing > 0 )
        {
          idx = count - 1;

          while ( idx >= 0 )
          {
            if ( columns.at( idx )->currentlyVisible() || ( idx == 0 ) )
            {
              int chop = lColumnWidths[ idx ] - lColumnSizeHints[ idx ];
              if ( chop > 0 )
              {
                if ( chop > missing )
                  chop = missing;
                lColumnWidths[ idx ] -= chop;
                missing -= chop;
                if ( missing < 1 )
                  break; // no more space to recover
              }
            } // else it's invisible
            idx--;
          }
        }
      }
    }
  }

  // We're ready to assign widths.



  bool oldSave = mSaveThemeColumnStateOnSectionResize;
  mSaveThemeColumnStateOnSectionResize = false;

  // A huge problem here is that QHeaderView goes quite nuts if we show or hide sections
  // while resizing them. This is because it has several machineries aimed to delay
  // the layout to the last possible moment. So if we show a column, it will tend to
  // screw up the layout of other ones.

  // We first loop showing/hiding columns then.

  idx = 0;

  for ( it = columns.begin(); it != columns.end(); ++it )
  {
    bool visible = ( idx == 0 ) || ( *it )->currentlyVisible();
    ( *it )->setCurrentlyVisible( visible );
    header()->setSectionHidden( idx, !visible );
    idx++;
  }

  // Then we loop assigning widths. This is still complicated since QHeaderView tries
  // very badly to stretch the last section and thus will resize it in the meantime.
  // But seems to work most of the times...

  idx = 0;
  totalVisibleWidth = 0;

  for ( it = columns.begin(); it != columns.end(); ++it )
  {
    if ( ( *it )->currentlyVisible() )
    {
      // kDebug() << "Resize section " << idx << " to " << lColumnWidths[ idx ];
      ( *it )->setCurrentWidth( lColumnWidths[ idx ] );
      header()->resizeSection( idx, lColumnWidths[ idx ] );
      totalVisibleWidth += lColumnWidths[ idx ];
    } else {
      ( *it )->setCurrentWidth( -1 );
    }
    idx++;
  }

  // kDebug() << "Total visible width after fixing is " << totalVisibleWidth << ", viewport width is " << viewport()->width();

  totalVisibleWidth = 0;
  idx = 0;

  bool bTriggeredQtBug = false;

  for ( QList< Theme::Column * >::ConstIterator it = columns.begin(); it != columns.end(); ++it )
  {
    if ( !header()->isSectionHidden( idx ) )
    {
      // kDebug() << "!! Final size for column " << idx << " is " << header()->sectionSize( idx );
      if ( !( *it )->currentlyVisible() )
      {
        bTriggeredQtBug = true;
        // kDebug() << "!! ERROR: Qt screwed up: I've set column " << idx << " to hidden but it's shown";
      }
      totalVisibleWidth += header()->sectionSize( idx );
    }
    idx++;
  }
  
  // kDebug() << "Total real visible width after fixing is " << totalVisibleWidth << ", viewport width is " << viewport()->width();

  setHeaderHidden( mTheme->viewHeaderPolicy() == Theme::NeverShowHeader );

  mSaveThemeColumnStateOnSectionResize = oldSave;
  mNeedToApplyThemeColumns = false;

  static bool bAllowRecursion = true;

  if (bTriggeredQtBug && bAllowRecursion)
  {
    bAllowRecursion = false;
    // kDebug() << "I've triggered the QHeaderView bug: trying to fix by calling myself again";
    applyThemeColumns();
    bAllowRecursion = true;
  }
}


void View::saveThemeColumnState()
{
  if ( mSaveThemeColumnStateTimer->isActive() )
    mSaveThemeColumnStateTimer->stop();

  if ( !mTheme )
    return;

  const QList< Theme::Column * > & columns = mTheme->columns();

  if ( columns.count() < 1 )
    return; // bad theme

  int idx = 0;


  for ( QList< Theme::Column * >::ConstIterator it = columns.begin(); it != columns.end(); ++it )
  {
    if ( header()->isSectionHidden( idx ) )
    {
      ( *it )->setCurrentlyVisible( false );
      ( *it )->setCurrentWidth( -1 ); // reset (hmmm... we could use the "don't touch" policy here too...)
    } else {
      ( *it )->setCurrentlyVisible( true );
      ( *it )->setCurrentWidth( header()->sectionSize( idx ) );
    }
    idx++;
  }
}

void View::triggerDelayedSaveThemeColumnState()
{
  if ( mSaveThemeColumnStateTimer->isActive() )
    mSaveThemeColumnStateTimer->stop();
  mSaveThemeColumnStateTimer->setSingleShot( true );
  mSaveThemeColumnStateTimer->start( 200 );
}


void View::resizeEvent( QResizeEvent * e )
{
  QTreeView::resizeEvent( e );

  if ( (!mFirstShow) && mNeedToApplyThemeColumns )
    applyThemeColumns();

  if ( header()->isVisible() )
    return;

  bool oldSave = mSaveThemeColumnStateOnSectionResize;
  mSaveThemeColumnStateOnSectionResize = false;

  // header invisible
  if ( ( header()->count() - header()->hiddenSectionCount() ) < 2 )
  {
    // a single column visible: resize it
    int visibleIndex;
    int count = header()->count();
    for ( visibleIndex = 0; visibleIndex < count; visibleIndex++ )
    {
      if ( !header()->isSectionHidden( visibleIndex ) )
        break;
    }

    if ( visibleIndex < count )
      header()->resizeSection( visibleIndex, viewport()->width() - 4 );
  }

  mSaveThemeColumnStateOnSectionResize = oldSave;
}

void View::modelAboutToEmitLayoutChanged()
{
  // QHeaderView goes totally NUTS with a layoutChanged() call
  mSaveThemeColumnStateOnSectionResize = false;
}

void View::modelEmittedLayoutChanged()
{
  // This is after a first chunk of work has been done by the model: do apply column states
  mSaveThemeColumnStateOnSectionResize = true;
  applyThemeColumns();
}

void View::slotHeaderSectionResized( int logicalIndex, int oldWidth, int newWidth )
{
  Q_UNUSED( logicalIndex );
  Q_UNUSED( oldWidth );
  Q_UNUSED( newWidth );

  if ( mSaveThemeColumnStateOnSectionResize )
    triggerDelayedSaveThemeColumnState();
}

int View::sizeHintForColumn( int logicalColumnIndex ) const
{
  // QTreeView: please don't touch my column widths...
  int w = header()->sectionSize( logicalColumnIndex );
  if ( w > 0 )
    return w;
  if ( !mDelegate )
    return 32; // dummy
  w = mDelegate->sizeHintForItemTypeAndColumn( Item::Message, logicalColumnIndex ).width();
  return w;
}

void View::showEvent( QShowEvent *e )
{
  QTreeView::showEvent( e );
  if ( mFirstShow )
  {
    //
    // If we're shown for the first time and the theme has been already set
    // then we need to reapply the theme column widths since the previous
    // application probably used invalid widths.
    //
    if ( mTheme )
      applyThemeColumns();
    mFirstShow = false;
  }
}

const int gHeaderContextMenuAdjustColumnSizesId = -1;
const int gHeaderContextMenuShowDefaultColumnsId = -2;

void View::slotHeaderContextMenuRequested( const QPoint &pnt )
{
  if ( !mTheme )
    return;

  const QList< Theme::Column * > & columns = mTheme->columns();

  if ( columns.count() < 1 )
    return; // bad theme

  // the menu for the columns
  KMenu menu;

  menu.addTitle( i18n( "Show Columns" ) );

  int idx = 0;
  QAction * act;

  for ( QList< Theme::Column * >::ConstIterator it = columns.begin(); it != columns.end(); ++it )
  {
    act = menu.addAction( ( *it )->label() );
    act->setCheckable( true );
    act->setChecked( !header()->isSectionHidden( idx ) );
    act->setData( QVariant( idx ) );
    if ( idx == 0)
       act->setEnabled( false );

    idx++;
  }

  menu.addSeparator();
  act = menu.addAction( i18n( "Adjust Column Sizes" ) );
  act->setData( QVariant( static_cast< int >( gHeaderContextMenuAdjustColumnSizesId ) ) );

  act = menu.addAction( i18n( "Show Default Columns" ) );
  act->setData( QVariant( static_cast< int >( gHeaderContextMenuShowDefaultColumnsId ) ) );

  QObject::connect(
      &menu, SIGNAL( triggered( QAction * ) ),
      this, SLOT( slotHeaderContextMenuTriggered( QAction *  ) )
    );

  menu.exec( header()->mapToGlobal( pnt ) );
}

void View::slotHeaderContextMenuTriggered( QAction * act )
{
  if ( !mTheme )
    return; // oops

  if ( !act )
    return;

  bool ok;
  int columnIdx = act->data().toInt( &ok );

  if ( !ok )
    return;

  if ( columnIdx < 0 )
  {
    if ( columnIdx == gHeaderContextMenuAdjustColumnSizesId )
    {
      // "Adjust Column Sizes"
      mTheme->resetColumnSizes();
      applyThemeColumns();
    } else if ( columnIdx == gHeaderContextMenuShowDefaultColumnsId )
    {
      // "Show Default Columns"
      mTheme->resetColumnState();
      applyThemeColumns();
    }
    return;
  }

  // Single column show or hide action
  if ( columnIdx == 0 )
    return; // can never be hidden

  if ( columnIdx >= mTheme->columns().count() )
    return;

  bool showIt = header()->isSectionHidden( columnIdx );

  Theme::Column * column = mTheme->columns().at( columnIdx );
  Q_ASSERT( column );

  // first save column state (as it is, with the column still in previous state)
  saveThemeColumnState();

  // If a section has just been shown, invalidate its width in the skin
  // since QTreeView assigned it a (possibly insane) default width.
  // If a section has been hidden, then invalidate its width anyway...
  // so finally invalidate width always, here.
  column->setCurrentlyVisible( showIt );
  column->setCurrentWidth( -1 );

  // then apply theme columns to re-compute proportional widths (so we hopefully stay in the view)
  applyThemeColumns();
}

MessageItem * View::currentMessageItem( bool selectIfNeeded ) const
{
  QModelIndex idx = currentIndex();
  if ( !idx.isValid() )
    return 0;
  Item * it = static_cast< Item * >( idx.internalPointer() );
  Q_ASSERT( it );
  if ( it->type() != Item::Message )
    return 0;

  if ( selectIfNeeded )
  {
    // Keep things coherent, if the user didn't select it, but acted on it via
    // a shortcut, do select it now.
    if ( !selectionModel()->isSelected( idx ) )
      selectionModel()->select( idx, QItemSelectionModel::Select | QItemSelectionModel::Current | QItemSelectionModel::Rows );
  }

  return static_cast< MessageItem * >( it );
}

void View::setCurrentMessageItem( MessageItem * it )
{
  if ( it )
    selectionModel()->setCurrentIndex( mModel->index( it, 0 ), QItemSelectionModel::Select | QItemSelectionModel::Current | QItemSelectionModel::Rows );
  else
    selectionModel()->setCurrentIndex( QModelIndex(), QItemSelectionModel::Current | QItemSelectionModel::Clear );
}

bool View::selectionEmpty() const
{
  return selectionModel()->selectedRows().isEmpty();
}

QList< MessageItem * > View::selectionAsMessageItemList( bool includeCollapsedChildren ) const
{
  QList< MessageItem * > selectedMessages;

  QModelIndexList lSelected = selectionModel()->selectedRows();
  if ( lSelected.isEmpty() )
    return selectedMessages;

  for ( QModelIndexList::Iterator it = lSelected.begin(); it != lSelected.end(); ++it )
  {
    // The asserts below are theoretically valid but at the time
    // of writing they fail because of a bug in QItemSelectionModel::selectedRows()
    // which returns also non-selectable items.

    //Q_ASSERT( selectedItem->type() == Item::Message );
    //Q_ASSERT( ( *it ).isValid() );

    if ( !( *it ).isValid() )
      continue;

    Item * selectedItem = static_cast< Item * >( ( *it ).internalPointer() );
    Q_ASSERT( selectedItem );

    if ( selectedItem->type() != Item::Message )
      continue;

    if ( !static_cast< MessageItem * >( selectedItem )->isValid() )
      continue;

    Q_ASSERT( !selectedMessages.contains( static_cast< MessageItem * >( selectedItem ) ) );

    if ( includeCollapsedChildren && ( selectedItem->childItemCount() > 0 ) && ( !isExpanded( *it ) ) )
    {
      static_cast< MessageItem * >( selectedItem )->subTreeToList( selectedMessages );
    } else {
      selectedMessages.append( static_cast< MessageItem * >( selectedItem ) );
    }
  }

  return selectedMessages;
}

QList< MessageItem * > View::currentThreadAsMessageItemList() const
{
  QList< MessageItem * > currentThread;

  MessageItem * msg = currentMessageItem();
  if ( !msg )
    return currentThread;

  while ( msg->parent() )
  {
    if ( msg->parent()->type() != Item::Message )
      break;
    msg = static_cast< MessageItem * >( msg->parent() );
  }

  msg->subTreeToList( currentThread );

  return currentThread;  
}

void View::setChildrenExpanded( const Item * root, bool expand )
{
  Q_ASSERT( root );
  QList< Item * > * childList = root->childItems();
  if ( !childList )
    return;
  for ( QList< Item * >::Iterator it = childList->begin(); it != childList->end(); ++it )
  {
    QModelIndex idx = mModel->index( *it, 0 );
    Q_ASSERT( idx.isValid() );
    Q_ASSERT( static_cast< Item * >( idx.internalPointer() ) == ( *it ) );

    if ( expand )
    {
      setExpanded( idx, true );

      if ( ( *it )->childItemCount() > 0 )
        setChildrenExpanded( *it, true );
    } else {
      if ( ( *it )->childItemCount() > 0 )
        setChildrenExpanded( *it, false );

      setExpanded( idx, false );
    }
  }
}

void View::setCurrentThreadExpanded( bool expand )
{
  MessageItem * message = currentMessageItem();
  if ( !message )
    return;

  while ( message->parent() )
  {
    if ( message->parent()->type() != Item::Message )
      break;
    message = static_cast< MessageItem * >( message->parent() );
  }

  if ( expand )
  {
    setExpanded( mModel->index( message, 0 ), true );
    setChildrenExpanded( message, true );
  } else {
    setChildrenExpanded( message, false );
    setExpanded( mModel->index( message, 0 ), false );
  }
}

void View::setAllThreadsExpanded( bool expand )
{
  setChildrenExpanded( mModel->rootItem(), expand );
}

void View::selectMessageItems( const QList< MessageItem * > &list )
{
  QItemSelection selection;
  for ( QList< MessageItem * >::ConstIterator it = list.constBegin(); it != list.constEnd(); ++it )
  {
    Q_ASSERT( *it );
    QModelIndex idx = mModel->index( *it, 0 );
    Q_ASSERT( idx.isValid() );
    Q_ASSERT( static_cast< MessageItem * >( idx.internalPointer() ) == ( *it ) );
    if ( !selectionModel()->isSelected( idx ) )
      selection.append( QItemSelectionRange( idx ) );
    ensureCurrentlyViewable( *it );
  }
  if ( !selection.isEmpty() )
    selectionModel()->select( selection, QItemSelectionModel::Select | QItemSelectionModel::Rows );
}

static inline bool message_type_matches( Item * item, MessageTypeFilter messageTypeFilter )
{
  switch( messageTypeFilter )
  {
    case MessageTypeAny:
      return true;
    break;
    case MessageTypeNewOnly:
      return item->status().isNew();
    break;
    case MessageTypeUnreadOnly:
      return item->status().isUnread();
    break;
    case MessageTypeNewOrUnreadOnly:
      return item->status().isNew() || item->status().isUnread();
    break;
    default:
      // nuthin here
    break;
  }

  // never reached
  Q_ASSERT( false );
  return false;
}

Item * View::messageItemAfter( Item * referenceItem, MessageTypeFilter messageTypeFilter, bool loop )
{
  if ( !storageModel() )
    return 0; // no folder

  // find the item to start with
  Item * below;

  if ( referenceItem )
  {
    // there was a current item: we start just below it
    if (
         ( referenceItem->childItemCount() > 0 )
         &&
         (
           ( messageTypeFilter != MessageTypeAny )
           ||
           isExpanded( mModel->index( referenceItem, 0 ) )
         )
       )
    {
      // the current item had children: either expanded or we want unread/new messages (and so we'll expand it if it isn't)
      below = referenceItem->itemBelow();
    } else {
      // the current item had no children: ask the parent to find the item below
      Q_ASSERT( referenceItem->parent() );
      below = referenceItem->parent()->itemBelowChild( referenceItem );
    }

    if ( !below )
    {
      // reached the end
      if ( loop )
      {
        // try re-starting from top
        below = mModel->rootItem()->itemBelow();
        Q_ASSERT( below ); // must exist (we had a current item)

        if ( below == referenceItem )
          return 0; // only one item in folder: loop complete
      } else {
        // looping not requested
        return 0;
      }
    }

  } else {
    // there was no current item, start from beginning
    below = mModel->rootItem()->itemBelow();

    if ( !below )
      return 0; // folder empty
  }

  // ok.. now below points to the next message.
  // While it doesn't satisfy our requirements, go further down

  while (
          // is not a message (we want messages, don't we ?)
          ( below->type() != Item::Message ) ||
          // message filter doesn't match
          ( !message_type_matches( below, messageTypeFilter ) ) ||
          // is hidden (and we don't want hidden items as they arent "officially" in the view)
          isRowHidden( below->parent()->indexOfChildItem( below ), mModel->index( below->parent(), 0 ) )
    )
  {
    // find the next one
    if ( ( below->childItemCount() > 0 ) && ( ( messageTypeFilter != MessageTypeAny ) || isExpanded( mModel->index( below, 0 ) ) ) )
    {
      // the current item had children: either expanded or we want unread messages (and so we'll expand it if it isn't)
      below = below->itemBelow();
    } else {
      // the current item had no children: ask the parent to find the item below
      Q_ASSERT( below->parent() );
      below = below->parent()->itemBelowChild( below );
    }

    if ( !below )
    {
      // we reached the end of the folder
      if ( loop )
      {
        // looping requested
        if ( referenceItem ) // <-- this means "we have started from something that is not the top: looping makes sense"
          below = mModel->rootItem()->itemBelow();
        // else mi == 0 and below == 0: we have started from the beginning and reached the end (it will fail the test below and exit)
      } else {
        // looping not requested: nothing more to do
        return 0;
      }
    }

    if( below == referenceItem )
    {
      Q_ASSERT( loop );
      return 0; // looped and returned back to the first message
    }
  }

  return below;
}

Item * View::nextMessageItem( MessageTypeFilter messageTypeFilter, bool loop )
{
  return messageItemAfter( currentMessageItem( false ), messageTypeFilter, loop );
}

Item * View::messageItemBefore( Item * referenceItem, MessageTypeFilter messageTypeFilter, bool loop )
{
  if ( !storageModel() )
    return 0; // no folder
 
  Item * above;

  if ( referenceItem )
  {
    // There was a current item, we start just above it
    above = referenceItem->itemAbove();

    if ( ( !above ) || ( above == mModel->rootItem() ) )
    {
      // reached the beginning
      if ( loop )
      {
        // try re-starting from bottom
        above = mModel->rootItem()->deepestItem();
        Q_ASSERT( above ); // must exist (we had a current item)
        Q_ASSERT( above != mModel->rootItem() );

        if ( above == referenceItem )
          return 0; // only one item in folder: loop complete
      } else {
        // looping not requested
        return 0;
      }

    }
  } else {
    // there was no current item, start from end
    above = mModel->rootItem()->deepestItem();

    if ( !above || ( above == mModel->rootItem() ) )
      return 0; // folder empty
  }

  // ok.. now below points to the previous message.
  // While it doesn't satisfy our requirements, go further up

  while (
          // is not a message (we want messages, don't we ?)
          ( above->type() != Item::Message ) ||
          // message filter doesn't match
          ( !message_type_matches( above, messageTypeFilter ) ) ||
          // we don't expand items but the item has parents unexpanded (so should be skipped)
          (
            // !expand items
            ( messageTypeFilter == MessageTypeAny ) &&
            // has unexpanded parents or is itself hidden
            ( ! isCurrentlyViewable( above ) )
          ) ||
          // is hidden
          isRowHidden( above->parent()->indexOfChildItem( above ), mModel->index( above->parent(), 0 ) )
    )
  {

    above = above->itemAbove();

    if ( ( !above ) || ( above == mModel->rootItem() ) )
    {
      // reached the beginning
      if ( loop )
      {
        // looping requested
        if ( referenceItem ) // <-- this means "we have started from something that is not the beginning: looping makes sense"
          above = mModel->rootItem()->deepestItem();
        // else mi == 0 and above == 0: we have started from the end and reached the beginning (it will fail the test below and exit)
      } else {
        // looping not requested: nothing more to do
        return 0;
      }
    }

    if( above == referenceItem )
    {
      Q_ASSERT( loop );
      return 0; // looped and returned back to the first message
    }
  }

  return above;
}

Item * View::previousMessageItem( MessageTypeFilter messageTypeFilter, bool loop )
{
  return messageItemBefore( currentMessageItem( false ), messageTypeFilter, loop );
}

void View::growOrShrinkExistingSelection( const QModelIndex &newSelectedIndex, bool movingUp )
{
  // Qt: why visualIndex() is private? ...I'd really need it here...

  int selectedVisualCoordinate = visualRect( newSelectedIndex ).top();

  int topVisualCoordinate = 0xfffffff; // huuuuuge number
  int bottomVisualCoordinate = -(0xfffffff);

  int candidate;

  QModelIndex bottomIndex;
  QModelIndex topIndex;

  // find out the actual selection range
  const QItemSelection selection = selectionModel()->selection();

  foreach ( QItemSelectionRange range, selection )
  {
    // We're asking the model for the index as range.topLeft() and range.bottomRight()
    // can return indexes in invisible columns which have a null visualRect().
    // Column 0, instead, is always visible.

    QModelIndex top = mModel->index( range.top(), 0, range.parent() );
    QModelIndex bottom = mModel->index( range.bottom(), 0, range.parent() );

    if ( top.isValid() )
    {
      if ( !bottom.isValid() )
        bottom = top;
    } else {
      if ( !top.isValid() )
        top = bottom;
    }
    candidate = visualRect( bottom ).bottom();
    if ( candidate > bottomVisualCoordinate )
    {
      bottomVisualCoordinate = candidate;
      bottomIndex = range.bottomRight();
    }

    candidate = visualRect( top ).top();
    if ( candidate < topVisualCoordinate )
    {
      topVisualCoordinate = candidate;
      topIndex = range.topLeft();
    }
  }


  if ( topIndex.isValid() && bottomIndex.isValid() )
  {
    if ( movingUp )
    {
      if ( selectedVisualCoordinate < topVisualCoordinate )
      {
        // selecting something above the top: grow selection
        selectionModel()->select( newSelectedIndex, QItemSelectionModel::Rows | QItemSelectionModel::Select );
      } else {
        // selecting something below the top: shrink selection
        QModelIndexList selectedIndexes = selection.indexes();
        foreach ( QModelIndex idx, selectedIndexes )
        {
          if ( ( idx.column() == 0 ) && ( visualRect( idx ).top() > selectedVisualCoordinate ) )
            selectionModel()->select( idx, QItemSelectionModel::Rows | QItemSelectionModel::Deselect );
        }
      }
    } else {
      if ( selectedVisualCoordinate > bottomVisualCoordinate )
      {
        // selecting something below bottom: grow selection
        selectionModel()->select( newSelectedIndex, QItemSelectionModel::Rows | QItemSelectionModel::Select );
      } else {
        // selecting something above bottom: shrink selection
        QModelIndexList selectedIndexes = selection.indexes();
        foreach ( QModelIndex idx, selectedIndexes )
        {
          if ( ( idx.column() == 0 ) && ( visualRect( idx ).top() < selectedVisualCoordinate ) )
            selectionModel()->select( idx, QItemSelectionModel::Rows | QItemSelectionModel::Deselect );
        }
      }
    }
  } else {
    // no existing selection, just grow
    selectionModel()->select( newSelectedIndex, QItemSelectionModel::Rows | QItemSelectionModel::Select );
  }
}

bool View::selectNextMessageItem(
    MessageTypeFilter messageTypeFilter,
    ExistingSelectionBehaviour existingSelectionBehaviour,
    bool centerItem,
    bool loop
  )
{
  Item * it = nextMessageItem( messageTypeFilter, loop );
  if ( !it )
    return false;

  setFocus();

  if ( it->parent() != mModel->rootItem() )
    ensureCurrentlyViewable( it );

  QModelIndex idx = mModel->index( it, 0 );

  Q_ASSERT( idx.isValid() );

  switch ( existingSelectionBehaviour )
  {
    case ExpandExistingSelection:
      selectionModel()->setCurrentIndex( idx, QItemSelectionModel::NoUpdate );
      selectionModel()->select( idx, QItemSelectionModel::Rows | QItemSelectionModel::Select );
    break;
    case GrowOrShrinkExistingSelection:
      selectionModel()->setCurrentIndex( idx, QItemSelectionModel::NoUpdate );
      growOrShrinkExistingSelection( idx, false );
    break;
    default:
    //case ClearExistingSelection:
      setCurrentIndex( idx );
    break;
  }

  if ( centerItem )
    scrollTo( idx, QAbstractItemView::PositionAtCenter );

  return true;
}

bool View::selectPreviousMessageItem(
    MessageTypeFilter messageTypeFilter,
    ExistingSelectionBehaviour existingSelectionBehaviour,
    bool centerItem,
    bool loop
  )
{
  Item * it = previousMessageItem( messageTypeFilter, loop );
  if ( !it )
    return false;

  setFocus();

  if ( it->parent() != mModel->rootItem() )
    ensureCurrentlyViewable( it );

  QModelIndex idx = mModel->index( it, 0 );

  Q_ASSERT( idx.isValid() );

  switch ( existingSelectionBehaviour )
  {
    case ExpandExistingSelection:
      selectionModel()->setCurrentIndex( idx, QItemSelectionModel::NoUpdate );
      selectionModel()->select( idx, QItemSelectionModel::Rows | QItemSelectionModel::Select );
    break;
    case GrowOrShrinkExistingSelection:
      selectionModel()->setCurrentIndex( idx, QItemSelectionModel::NoUpdate );
      growOrShrinkExistingSelection( idx, true );
    break;
    default:
    //case ClearExistingSelection:
      setCurrentIndex( idx );
    break;
  }

  if ( centerItem )
    scrollTo( idx, QAbstractItemView::PositionAtCenter );

  return true;
}

bool View::focusNextMessageItem( MessageTypeFilter messageTypeFilter, bool centerItem, bool loop )
{
  Item * it = nextMessageItem( messageTypeFilter, loop );
  if ( !it )
    return false;

  setFocus();

  if ( it->parent() != mModel->rootItem() )
    ensureCurrentlyViewable( it );

  QModelIndex idx = mModel->index( it, 0 );

  Q_ASSERT( idx.isValid() );

  selectionModel()->setCurrentIndex( idx, QItemSelectionModel::NoUpdate );

  if ( centerItem )
    scrollTo( idx, QAbstractItemView::PositionAtCenter );

  return true;
}

bool View::focusPreviousMessageItem( MessageTypeFilter messageTypeFilter, bool centerItem, bool loop )
{
  Item * it = previousMessageItem( messageTypeFilter, loop );
  if ( !it )
    return false;

  setFocus();

  if ( it->parent() != mModel->rootItem() )
    ensureCurrentlyViewable( it );

  QModelIndex idx = mModel->index( it, 0 );

  Q_ASSERT( idx.isValid() );

  selectionModel()->setCurrentIndex( idx, QItemSelectionModel::NoUpdate );

  if ( centerItem )
    scrollTo( idx, QAbstractItemView::PositionAtCenter );

  return true;
}

void View::selectFocusedMessageItem( bool centerItem )
{
  QModelIndex idx = currentIndex();
  if ( !idx.isValid() )
    return;

  setFocus();

  if ( selectionModel()->isSelected( idx ) )
    return;

  selectionModel()->select( idx, QItemSelectionModel::Select | QItemSelectionModel::Current | QItemSelectionModel::Rows );

  if ( centerItem )
    scrollTo( idx, QAbstractItemView::PositionAtCenter );
}

void View::applyMessagePreSelection( PreSelectionMode preSelectionMode )
{
  mModel->applyMessagePreSelection( preSelectionMode );
}


bool View::selectFirstMessageItem( MessageTypeFilter messageTypeFilter, bool centerItem )
{
  if ( !storageModel() )
    return false; // nothing to do

  Item * it = firstMessageItem( messageTypeFilter );
  if ( !it )
    return false;

  Q_ASSERT( it != mModel->rootItem() ); // must never happen (obviously)

  setFocus();
  ensureCurrentlyViewable( it );

  QModelIndex idx = mModel->index( it, 0 );

  Q_ASSERT( idx.isValid() );

  setCurrentIndex( idx );

  if ( centerItem )
    scrollTo( idx, QAbstractItemView::PositionAtCenter );

  return true;
}

void View::modelFinishedLoading()
{
  Q_ASSERT( storageModel() );
  Q_ASSERT( !mModel->isLoading() );

  // nothing here for now :)
}

MessageItemSetReference View::createPersistentSet( const QList< MessageItem * > &items )
{
  return mModel->createPersistentSet( items );
}

QList< MessageItem * > View::persistentSetCurrentMessageItemList( MessageItemSetReference ref )
{
  return mModel->persistentSetCurrentMessageItemList( ref );
}

void View::deletePersistentSet( MessageItemSetReference ref )
{
  mModel->deletePersistentSet( ref );
}

void View::markMessageItemsAsAboutToBeRemoved( QList< MessageItem * > &items, bool bMark )
{
  if ( bMark )
  {
    for ( QList< MessageItem * >::Iterator it = items.begin(); it != items.end(); ++it )
    {
      ( *it )->setAboutToBeRemoved( true );
      QModelIndex idx = mModel->index( *it, 0 );
      Q_ASSERT( idx.isValid() );
      Q_ASSERT( static_cast< MessageItem * >( idx.internalPointer() ) == *it );
      if ( selectionModel()->isSelected( idx ) )
        selectionModel()->select( idx, QItemSelectionModel::Deselect | QItemSelectionModel::Rows );
    }
  } else {
    for ( QList< MessageItem * >::Iterator it = items.begin(); it != items.end(); ++it )
    {
      if ( ( *it )->isValid() ) // hasn't been removed in the meantime
        ( *it )->setAboutToBeRemoved( false );
    }
  }

  viewport()->update();
}

void View::ensureCurrentlyViewable( Item * it )
{
  Q_ASSERT( it );
  Q_ASSERT( it->parent() );
  Q_ASSERT( it->isViewable() ); // must be attached to the viewable root

  if ( isRowHidden( it->parent()->indexOfChildItem( it ), mModel->index( it->parent(), 0 ) ) )
    setRowHidden( it->parent()->indexOfChildItem( it ), mModel->index( it->parent(), 0 ), false );

  it = it->parent();

  while ( it->parent() )
  {
    if ( isRowHidden( it->parent()->indexOfChildItem( it ), mModel->index( it->parent(), 0 ) ) )
      setRowHidden( it->parent()->indexOfChildItem( it ), mModel->index( it->parent(), 0 ), false );

    QModelIndex idx = mModel->index( it, 0 );

    Q_ASSERT( idx.isValid() );
    Q_ASSERT( static_cast< Item * >( idx.internalPointer() ) == it );

    if ( !isExpanded( idx ) )
      setExpanded( idx, true );

    it = it->parent();
  }
}

bool View::isCurrentlyViewable( Item * it ) const
{
  Q_ASSERT( it );
  Q_ASSERT( it->parent() );

  if ( !it->isViewable() )
    return false;
  if ( isRowHidden( it->parent()->indexOfChildItem( it ), mModel->index( it->parent(), 0 ) ) )
    return false;

  it = it->parent();

  if ( it == mModel->rootItem() )
    return true;

  while ( it )
  {
    if ( it->parent() == mModel->rootItem() )
      return true;
    if ( !isExpanded( mModel->index( it, 0 ) ) )
      return false;
    it = it->parent();
  }
  return false;
}

bool View::isThreaded() const
{
  if ( !mAggregation )
    return false;
  return mAggregation->threading() != Aggregation::NoThreading;
}

void View::slotSelectionChanged( const QItemSelection &, const QItemSelection & )
{
  // We assume that when selection changes, current item also changes.
  QModelIndex current = currentIndex();

  // Abort any pending message pre-selection as the user is probably
  // already navigating the view (so pre-selection would make his view jump
  // to an unexpected place).
  mModel->abortMessagePreSelection();

  if ( !current.isValid() )
  {
    if ( mLastCurrentItem )
    {
      mWidget->viewMessageSelected( 0 );
      mLastCurrentItem = 0;
    }
    mWidget->viewMessageSelected( 0 );
    mWidget->viewSelectionChanged();
    return;
  }

  if ( !selectionModel()->isSelected( current ) )
  {
    if ( selectedIndexes().count() < 1 )
    {
      // It may happen after row removals: Model calls this slot on currentIndex()
      // that actually might have changed "silently", without being selected.
      QItemSelection selection;
      selection.append( QItemSelectionRange( current ) );
      selectionModel()->select( selection, QItemSelectionModel::Select | QItemSelectionModel::Rows );
    } else {
      // something is still selected anyway
      // This is probably a result of CTRL+Click which unselected current: leave it as it is.
      return;
    }
  }

  Item * it = static_cast< Item * >( current.internalPointer() );
  Q_ASSERT( it );

  switch ( it->type() )
  {
    case Item::Message:
    {
      if ( mLastCurrentItem != it )
      {
        kDebug() << "Message selected [" << it->subject().toUtf8().data() << "]" << endl;
        mWidget->viewMessageSelected( static_cast< MessageItem * >( it ) );
        mLastCurrentItem = 0;
      }
    } 
    break;
    case Item::GroupHeader:
      if ( mLastCurrentItem )
      {
        mWidget->viewMessageSelected( 0 );
        mLastCurrentItem = 0;
      }
    break;
    default:
      // should never happen
      Q_ASSERT( false );
    break;
  }

  mWidget->viewSelectionChanged();
}

void View::mouseDoubleClickEvent( QMouseEvent * e )
{
  // Perform a hit test
  if ( !mDelegate->hitTest( e->pos(), true ) )
    return;

  // Something was hit :)

  Item * it = static_cast< Item * >( mDelegate->hitItem() );
  if ( !it )
    return; // should never happen

  switch ( it->type() )
  {
    case Item::Message:
    {
      // Let QTreeView handle the expansion
      QTreeView::mousePressEvent( e );

      switch ( e->button() )
      {
        case Qt::LeftButton:

          if ( mDelegate->hitContentItem() )
          {
            // Double clikcking on clickable icons does NOT activate the message
            if ( mDelegate->hitContentItem()->isIcon() && mDelegate->hitContentItem()->isClickable() )
              return;
          }

          mWidget->viewMessageActivated( static_cast< MessageItem * >( it ) );
        break;
        default:
          // make gcc happy
        break;
      }
    } 
    break;
    case Item::GroupHeader:
    {
      // Don't let QTreeView handle the selection (as it deselects the curent messages)
      switch ( e->button() )
      {
        case Qt::LeftButton:
          if ( it->childItemCount() > 0 )
          {
            // toggle expanded state
            setExpanded( mDelegate->hitIndex(), !isExpanded( mDelegate->hitIndex() ) );
          }
        break;
        default:
          // make gcc happy
        break;
      }
    }
    break;
    default:
      // should never happen
      Q_ASSERT( false );
    break;
  }
}

void View::changeMessageStatus( MessageItem * it, const KPIM::MessageStatus &set, const KPIM::MessageStatus &unset )
{
  // We first change the status of MessageItem itself. This will make the change
  // visible to the user even if the Model is actually in the middle of a long job (maybe it's loading)
  // and can't process the status change request immediately.
  // Here we actually desynchronize the cache and trust that the later call to
  // mWidget->viewMessageStatusChangeRequest() will really perform the status change on the storage.
  // Well... in KMail it will unless something is really screwed. Anyway, if it will not, at the next
  // load the status will be just unchanged: no animals will be harmed.

  qint32 stat = it->status().toQInt32();
  stat |= set.toQInt32();
  stat &= ~( unset.toQInt32() );
  KPIM::MessageStatus status;
  status.fromQInt32( stat );
  it->setStatus( status );

  // Trigger an update so the immediate change will be shown to the user

  viewport()->update();

  // This will actually request the widget to perform a status change on the storage.
  // The request will be then processed by the Model and the message will be updated again.

  mWidget->viewMessageStatusChangeRequest( it, set, unset );
}

void View::mousePressEvent( QMouseEvent * e )
{
  mMousePressPosition = QPoint();

  // Perform a hit test
  if ( !mDelegate->hitTest( e->pos(), true ) )
    return;

  // Something was hit :)

  Item * it = static_cast< Item * >( mDelegate->hitItem() );
  if ( !it )
    return; // should never happen

  switch ( it->type() )
  {
    case Item::Message:
    {
      mMousePressPosition = e->pos();

      switch ( e->button() )
      {
        case Qt::LeftButton:
          // if we have multi selection then the meaning of hitting
          // the content item is quite unclear.
          if ( mDelegate->hitContentItem() && ( selectedIndexes().count() > 1 ) )
          {
            kDebug() << "Left hit with selectedIndexes().count() == " << selectedIndexes().count();

            switch ( mDelegate->hitContentItem()->type() )
            {
              case Theme::ContentItem::ActionItemStateIcon:
                changeMessageStatus(
                    static_cast< MessageItem * >( it ),
                    it->status().isToAct() ? KPIM::MessageStatus() : KPIM::MessageStatus::statusToAct(),
                    it->status().isToAct() ? KPIM::MessageStatus::statusToAct() : KPIM::MessageStatus()
                  );
                return; // don't select the item  
              break;
              case Theme::ContentItem::ImportantStateIcon:
                changeMessageStatus(
                    static_cast< MessageItem * >( it ),
                    it->status().isImportant() ? KPIM::MessageStatus() : KPIM::MessageStatus::statusImportant(),
                    it->status().isImportant() ? KPIM::MessageStatus::statusImportant() : KPIM::MessageStatus()
                  );
                return; // don't select the item  
              break;
              case Theme::ContentItem::SpamHamStateIcon:
                changeMessageStatus(
                    static_cast< MessageItem * >( it ),
                    it->status().isSpam() ? KPIM::MessageStatus() : ( it->status().isHam() ? KPIM::MessageStatus::statusSpam() : KPIM::MessageStatus::statusHam() ),
                    it->status().isSpam() ? KPIM::MessageStatus::statusSpam() : ( it->status().isHam() ? KPIM::MessageStatus::statusHam() : KPIM::MessageStatus() )
                  );
                return; // don't select the item  
              break;
              case Theme::ContentItem::WatchedIgnoredStateIcon:
                changeMessageStatus(
                    static_cast< MessageItem * >( it ),
                    it->status().isIgnored() ? KPIM::MessageStatus() : ( it->status().isWatched() ? KPIM::MessageStatus::statusIgnored() : KPIM::MessageStatus::statusWatched() ),
                    it->status().isIgnored() ? KPIM::MessageStatus::statusIgnored() : ( it->status().isWatched() ? KPIM::MessageStatus::statusWatched() : KPIM::MessageStatus() )
                  );
                return; // don't select the item  
              break;
              default:
                // make gcc happy
              break;
            }
          }

          // Let QTreeView handle the selection and emit the appropriate signals (slotSelectionChanged() may be called)
          QTreeView::mousePressEvent( e );

        break;
        case Qt::RightButton:
          // Let QTreeView handle the selection and emit the appropriate signals (slotSelectionChanged() may be called)
          QTreeView::mousePressEvent( e );

          mWidget->viewMessageListContextPopupRequest( selectionAsMessageItemList(), viewport()->mapToGlobal( e->pos() ) );
        break;
        default:
          // make gcc happy
        break;
      }
    } 
    break;
    case Item::GroupHeader:
    {
      // Don't let QTreeView handle the selection (as it deselects the curent messages)
      GroupHeaderItem *groupHeaderItem = static_cast< GroupHeaderItem * >( it );

      switch ( e->button() )
      {
        case Qt::LeftButton:
          if ( !mDelegate->hitContentItem() )
            return;

          if ( mDelegate->hitContentItem()->type() == Theme::ContentItem::ExpandedStateIcon )
          {
            if ( groupHeaderItem->childItemCount() > 0 )
            {
              // toggle expanded state
              setExpanded( mDelegate->hitIndex(), !isExpanded( mDelegate->hitIndex() ) );
            }
          }
        break;
        case Qt::RightButton:
          clearSelection(); // make sure it's true, so it's clear that the eventual popup belongs to the group header
          mWidget->viewGroupHeaderContextPopupRequest( groupHeaderItem, viewport()->mapToGlobal( e->pos() ) );
        break;
        default:
          // make gcc happy
        break;
      }
    }
    break;
    default:
      // should never happen
      Q_ASSERT( false );
    break;
  }
}

void View::mouseMoveEvent( QMouseEvent * e )
{
  if ( !e->buttons() & Qt::LeftButton )
  {
    QTreeView::mouseMoveEvent( e );
    return;
  }

  if ( mMousePressPosition.isNull() )
    return;

  if ( ( e->pos() - mMousePressPosition ).manhattanLength() <= KGlobalSettings::dndEventDelay() )
    return;

  mWidget->viewStartDragRequest();
}

void View::dragEnterEvent( QDragEnterEvent * e )
{
  mWidget->viewDragEnterEvent( e );
}

void View::dragMoveEvent( QDragMoveEvent * e )
{
  mWidget->viewDragMoveEvent( e );
}

void View::dropEvent( QDropEvent * e )
{
  mWidget->viewDropEvent( e );
}

void View::changeEvent( QEvent *e )
{
  switch ( e->type() )
  {
    case QEvent::PaletteChange:
    case QEvent::FontChange:
    case QEvent::StyleChange:
    case QEvent::LayoutDirectionChange:
    case QEvent::LocaleChange:
    case QEvent::LanguageChange:
      // All of these affect the theme's internal cache.
      setTheme( mTheme );
      // A layoutChanged() event will screw up the view state a bit.
      // Since this is a rare event we just reload the view.
      reload();
    break;
    default:
      // make gcc happy by default
    break;
  }

  QTreeView::changeEvent( e );
}

bool View::event( QEvent *e )
{
  // We catch ToolTip events and pass everything else

  if( e->type() != QEvent::ToolTip )
    return QTreeView::event( e );

  QHelpEvent * he = dynamic_cast< QHelpEvent * >( e );
  if ( !he )
    return true; // eh ?

  if ( !Manager::instance()->displayMessageToolTips() )
    return true; // don't display tooltips

  QPoint pnt = viewport()->mapFromGlobal( mapToGlobal( he->pos() ) );

  if ( pnt.y() < 0 )
    return true; // don't display the tooltip for items hidden under the header

  QModelIndex idx = indexAt( pnt );
  if ( !idx.isValid() )
    return true; // may be

  Item * it = static_cast< Item * >( idx.internalPointer() );
  if ( !it )
    return true; // hum

  Q_ASSERT( storageModel() );

  QColor bckColor = palette().color( QPalette::ToolTipBase );
  QColor txtColor = palette().color( QPalette::ToolTipText );
  QColor darkerColor(
      ( ( bckColor.red() * 8 ) + ( txtColor.red() * 2 ) ) / 10,
      ( ( bckColor.green() * 8 ) + ( txtColor.green() * 2 ) ) / 10,
      ( ( bckColor.blue() * 8 ) + ( txtColor.blue() * 2 ) ) / 10
    );

  QString bckColorName = bckColor.name();
  QString txtColorName = txtColor.name();
  QString darkerColorName = darkerColor.name();


  QString tip = QString::fromLatin1(
      "<table width=\"100%\" border=\"0\" cellpadding=\"2\" cellspacing=\"0\">"
    );

  switch ( it->type() )
  {
    case Item::Message:
    {
      MessageItem *mi = static_cast< MessageItem * >( it );

      tip += QString::fromLatin1(
           "<tr>" \
              "<td bgcolor=\"%1\" align=\"left\" valign=\"middle\">" \
                "<div style=\"color: %2; font-weight: bold;\">" \
                 "%3" \
                "</div>" \
              "</td>" \
            "</tr>"
        ).arg( txtColorName ).arg( bckColorName ).arg( mi->subject() );

      tip += QString::fromLatin1(
           "<tr>" \
              "<td align=\"center\" valign=\"middle\">" \
                "<table width=\"100%\" border=\"0\" cellpadding=\"2\" cellspacing=\"0\">"
        );

      tip += QString::fromLatin1(
           "<tr>" \
              "<td align=\"right\" valign=\"top\" width=\"45\">" \
                "<div style=\"font-weight: bold;\"><nobr>" \
                 "%1:" \
                "</nobr></div>" \
              "</td>" \
              "<td align=\"left\" valign=\"top\">" \
                 "%2" \
              "</td>" \
            "</tr>"
        ).arg( i18n( "From" ) ).arg( mi->sender() );

      tip += QString::fromLatin1(
           "<tr>" \
              "<td align=\"right\" valign=\"top\" width=\"45\">" \
                "<div style=\"font-weight: bold;\"><nobr>" \
                 "%1:" \
                "</nobr></div>" \
              "</td>" \
              "<td align=\"left\" valign=\"top\">" \
                 "%2" \
              "</td>" \
            "</tr>"
        ).arg( i18nc( "Receiver of the emial", "To" ) ).arg( mi->receiver() );

      tip += QString::fromLatin1(
           "<tr>" \
              "<td align=\"right\" valign=\"top\" width=\"45\">" \
                "<div style=\"font-weight: bold;\"><nobr>" \
                 "%1:" \
                "</nobr></div>" \
              "</td>" \
              "<td align=\"left\" valign=\"top\">" \
                 "%2" \
              "</td>" \
            "</tr>"
        ).arg( i18n( "Date" ) ).arg( mi->formattedDate() );

      tip += QString::fromLatin1(
           "<tr>" \
              "<td align=\"right\" valign=\"top\" width=\"45\">" \
                "<div style=\"font-weight: bold;\"><nobr>" \
                 "%1:" \
                "</nobr></div>" \
              "</td>" \
              "<td align=\"left\" valign=\"top\">" \
                 "%2" \
              "</td>" \
            "</tr>"
        ).arg( i18n( "Status" ) ).arg( mi->statusDescription() );

      tip += QString::fromLatin1(
           "<tr>" \
              "<td align=\"right\" valign=\"top\" width=\"45\">" \
                "<div style=\"font-weight: bold;\"><nobr>" \
                 "%1:" \
                "</nobr></div>" \
              "</td>" \
              "<td align=\"left\" valign=\"top\">" \
                 "%2" \
              "</td>" \
            "</tr>"
        ).arg( i18n( "Size" ) ).arg( mi->formattedSize() );


      tip += QString::fromLatin1(
                "</table" \
              "</td>" \
            "</tr>"
        );

      // FIXME: Find a way to show also CC and other header fields ?
      //        Text-mail 2 line preview would be also nice.. but that's kinda hard...

      if ( mi->hasChildren() )
      {
        Item::ChildItemStats stats;
        mi->childItemStats( stats );

        QString statsText;

        statsText = i18np( "<b>%1</b> reply", "<b>%1</b> replies", mi->childItemCount() );
        statsText += QLatin1String( ", " );

        statsText += i18np(
            "<b>%1</b> message in subtree (<b>%2</b> new + <b>%3</b> unread)",
            "<b>%1</b> messages in subtree (<b>%2</b> new + <b>%3</b> unread)",
            stats.mTotalChildCount,
            stats.mNewChildCount,
            stats.mUnreadChildCount
          );


        tip += QString::fromLatin1(
             "<tr>" \
                "<td bgcolor=\"%1\" align=\"left\" valign=\"middle\">" \
                   "<nobr>%2</nobr>" \
                "</td>" \
              "</tr>"
          ).arg( darkerColorName ).arg( statsText );
      }

    }
    break;
    case Item::GroupHeader:
    {
      GroupHeaderItem *ghi = static_cast< GroupHeaderItem * >( it );

      tip += QString::fromLatin1(
           "<tr>" \
              "<td bgcolor=\"%1\" align=\"left\" valign=\"middle\">" \
                "<div style=\"color: %2; font-weight: bold;\">" \
                 "%3" \
                "</div>" \
              "</td>" \
            "</tr>"
        ).arg( txtColorName ).arg( bckColorName ).arg( ghi->label() );

      QString description;

      switch( mAggregation->grouping() )
      {
        case Aggregation::GroupByDate:
          if ( mAggregation->threading() != Aggregation::NoThreading )
          {
            switch ( mAggregation->threadLeader() )
            {
              case Aggregation::TopmostMessage:
                if ( ghi->label().contains( QRegExp( "[0-9]" ) ) )
                  description = i18nc(
                      "@info:tooltip Formats to something like 'Threads started on 2008-12-21'",
                      "Threads started on %1",
                      ghi->label()
                    );
                else
                  description = i18nc(
                      "@info:tooltip Formats to something like 'Threads started Yesterday'",
                      "Threads started %1",
                      ghi->label()
                    );
              break;
              case Aggregation::MostRecentMessage:
                description = i18n( "Threads with messages dated %1", ghi->label() );
              break;
              default:
                // nuthin, make gcc happy
              break;
            }
          } else {
            if ( ghi->label().contains( QRegExp( "[0-9]" ) ) )
            {
              if ( storageModel()->containsOutboundMessages() )
                description = i18nc(
                    "@info:tooltip Formats to something like 'Messages sent on 2008-12-21'",
                    "Messages sent on %1",
                    ghi->label()
                  );
              else
                description = i18nc(
                    "@info:tooltip Formats to something like 'Messages received on 2008-12-21'",
                    "Messages received on %1",
                    ghi->label()
                  );
            } else {
              if ( storageModel()->containsOutboundMessages() )
                description = i18nc(
                    "@info:tooltip Formats to something like 'Messages sent Yesterday'",
                    "Messages sent %1",
                    ghi->label()
                  );
              else
                description = i18nc(
                    "@info:tooltip Formats to something like 'Messages received Yesterday'",
                    "Messages received %1",
                    ghi->label()
                  );
            }
          }
        break;
        case Aggregation::GroupByDateRange:
          if ( mAggregation->threading() != Aggregation::NoThreading )
          {
            switch ( mAggregation->threadLeader() )
            {
              case Aggregation::TopmostMessage:
                description = i18n( "Threads started within %1", ghi->label() );
              break;
              case Aggregation::MostRecentMessage:
                description = i18n( "Threads containing messages with dates within %1", ghi->label() );
              break;
              default:
                // nuthin, make gcc happy
              break;
            }
          } else {
            if ( storageModel()->containsOutboundMessages() )
              description = i18n( "Messages sent within %1", ghi->label() );
            else
              description = i18n( "Messages received within %1", ghi->label() );
          }
        break;
        case Aggregation::GroupBySenderOrReceiver:
        case Aggregation::GroupBySender:
          if ( mAggregation->threading() != Aggregation::NoThreading )
          {
            switch ( mAggregation->threadLeader() )
            {
              case Aggregation::TopmostMessage:
                description = i18n( "Threads started by %1", ghi->label() );
              break;
              case Aggregation::MostRecentMessage:
                description = i18n( "Threads with most recent message by %1", ghi->label() );
              break;
              default:
                // nuthin, make gcc happy
              break;
            }
          } else {
            if ( storageModel()->containsOutboundMessages() )
            {
              if ( mAggregation->grouping() == Aggregation::GroupBySenderOrReceiver )
                description = i18n( "Messages sent to %1", ghi->label() );
              else
                description = i18n( "Messages sent by %1", ghi->label() );
            } else {
              description = i18n( "Messages received from %1", ghi->label() );
            }
          }
        break;
        case Aggregation::GroupByReceiver:
          if ( mAggregation->threading() != Aggregation::NoThreading )
          {
            switch ( mAggregation->threadLeader() )
            {
              case Aggregation::TopmostMessage:
                description = i18n( "Threads directed to %1", ghi->label() );
              break;
              case Aggregation::MostRecentMessage:
                description = i18n( "Threads with most recent message directed to %1", ghi->label() );
              break;
              default:
                // nuthin, make gcc happy
              break;
            }
          } else {
            if ( storageModel()->containsOutboundMessages() )
            {
              description = i18n( "Messages sent to %1", ghi->label() );
            } else {
              description = i18n( "Messages received by %1", ghi->label() );
            }
          }
        break;
        default:
          // nuthin, make gcc happy
        break;
      }

      if ( !description.isEmpty() )
      {
        tip += QString::fromLatin1(
             "<tr>" \
                "<td align=\"left\" valign=\"middle\">" \
                   "%1" \
                "</td>" \
              "</tr>"
          ).arg( description );
      }

      if ( ghi->hasChildren() )
      {
        Item::ChildItemStats stats;
        ghi->childItemStats( stats );

        QString statsText;

        if ( mAggregation->threading() != Aggregation::NoThreading )
        {
          statsText = i18np( "<b>%1</b> thread", "<b>%1</b> threads", ghi->childItemCount() );
          statsText += QLatin1String( ", " );
        }

        statsText += i18np(
            "<b>%1</b> message (<b>%2</b> new + <b>%3</b> unread)",
            "<b>%1</b> messages (<b>%2</b> new + <b>%3</b> unread)",
            stats.mTotalChildCount,
            stats.mNewChildCount,
            stats.mUnreadChildCount
          );

        tip += QString::fromLatin1(
             "<tr>" \
                "<td bgcolor=\"%1\" align=\"left\" valign=\"middle\">" \
                   "<nobr>%2</nobr>" \
                "</td>" \
              "</tr>"
          ).arg( darkerColorName ).arg( statsText );
      }

    }
    break;
    default:
      // nuthin (just make gcc happy for now)
    break;
  }


  tip += QString::fromLatin1(
      "</table>"
    );

  QToolTip::showText( he->globalPos(), tip, viewport(), visualRect( idx ) );

  return true;  
}

void View::slotCollapseAllGroups()
{
  if ( mAggregation->grouping() == Aggregation::NoGrouping )
    return;

  Item * item = mModel->rootItem();

  QList< Item * > * childList = item->childItems();
  if ( !childList )
    return;

  for ( QList< Item * >::Iterator it = childList->begin(); it != childList->end(); ++it )
  {
    Q_ASSERT( ( *it )->type() == Item::GroupHeader );
    QModelIndex idx = mModel->index( *it, 0 );
    Q_ASSERT( idx.isValid() );
    Q_ASSERT( static_cast< Item * >( idx.internalPointer() ) == ( *it ) );
    if ( isExpanded( idx ) )
      setExpanded( idx, false );
  }
}

void View::slotExpandAllGroups()
{
  if ( mAggregation->grouping() == Aggregation::NoGrouping )
    return;

  Item * item = mModel->rootItem();

  QList< Item * > * childList = item->childItems();
  if ( !childList )
    return;

  for ( QList< Item * >::Iterator it = childList->begin(); it != childList->end(); ++it )
  {
    Q_ASSERT( ( *it )->type() == Item::GroupHeader );
    QModelIndex idx = mModel->index( *it, 0 );
    Q_ASSERT( idx.isValid() );
    Q_ASSERT( static_cast< Item * >( idx.internalPointer() ) == ( *it ) );
    if ( !isExpanded( idx ) )
      setExpanded( idx, true );
  }
}

} // namespace Core

} // namespace MessageListView

} // namespace KMail


#if 0

void KMHeaders::setFolder( KMFolder *aFolder, bool forceJumpToUnread )
{
    if (mFolder) {
      disconnect(mFolder, SIGNAL(numUnreadMsgsChanged(KMFolder*)),
          this, SLOT(setFolderInfoStatus()));
      disconnect(mFolder, SIGNAL(changed()),
                 this, SLOT(msgChanged()));
      disconnect( mFolder, SIGNAL( statusMsg( const QString& ) ),
                  BroadcastStatus::instance(), SLOT( setStatusMsg( const QString& ) ) );
      disconnect(mFolder, SIGNAL(viewConfigChanged()), this, SLOT(reset()));
    }

    mOwner->useAction()->setEnabled( mFolder ?
                         ( kmkernel->folderIsTemplates( mFolder ) ) : false );
    mOwner->messageActions()->replyListAction()->setEnabled( mFolder ?
                         mFolder->isMailingListEnabled() : false );
    if (mFolder)
    {
      connect(mFolder, SIGNAL(changed()),
              this, SLOT(msgChanged()));
      connect(mFolder, SIGNAL(statusMsg(const QString&)),
              BroadcastStatus::instance(), SLOT( setStatusMsg( const QString& ) ) );
      connect(mFolder, SIGNAL(numUnreadMsgsChanged(KMFolder*)),
          this, SLOT(setFolderInfoStatus()));
      connect(mFolder, SIGNAL(viewConfigChanged()), this, SLOT(reset()));
    }

    colText = i18n( "Date" );
    if (mPaintInfo.orderOfArrival)
      colText = i18n( "Date (Order of Arrival)" );
    setColumnText( mPaintInfo.dateCol, colText);
  }
}

void KMHeaders::setFolderInfoStatus ()
{
  if ( !mFolder ) return;
  QString str;
  const int unread = mFolder->countUnread();
  if ( static_cast<KMFolder*>(mFolder) == kmkernel->outboxFolder() )
    str = unread ? i18ncp( "Number of unsent messages", "1 unsent", "%1 unsent", unread ) : i18n( "0 unsent" );
  else
    str = unread ? i18ncp( "Number of unread messages", "1 unread", "%1 unread", unread )
      : i18nc( "No unread messages", "0 unread" );
  const int count = mFolder->count();
  str = count ? i18ncp( "Number of unread messages", "1 message, %2.", "%1 messages, %2.", count, str )
              : i18nc( "No unread messages", "0 messages" ); // no need for "0 unread" to be added here
  if ( mFolder->isReadOnly() )
    str = i18nc("%1 = n messages, m unread.", "%1 Folder is read-only.", str );
  BroadcastStatus::instance()->setStatusMsg(str);
}

#endif

