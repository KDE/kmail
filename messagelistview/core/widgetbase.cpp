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

#include "messagelistview/core/widgetbase.h"

#include "messagelistview/core/aggregation.h"
#include "messagelistview/core/theme.h"
#include "messagelistview/core/filter.h"
#include "messagelistview/core/manager.h"
#include "messagelistview/core/optionset.h"
#include "messagelistview/core/view.h"
#include "messagelistview/core/model.h"
#include "messagelistview/core/messageitem.h"
#include "messagelistview/core/storagemodelbase.h"

#include <QGridLayout>
#include <QVariant>
#include <QToolButton>
#include <QActionGroup>
#include <QTimer>
#include <QHeaderView>

#include <KMenu>
#include <KConfig>
#include <KDebug>
#include <KIconLoader>
#include <KLineEdit>
#include <KIcon>
#include <KLocale>
#include <KComboBox>

#include <libkdepim/messagestatus.h>

namespace KMail
{

namespace MessageListView
{

namespace Core
{

Widget::Widget( QWidget *pParent )
  : QWidget( pParent ),
    mStorageModel( 0 ),
    mAggregation( 0 ),
    mTheme( 0 ),
    mFilter( 0 ),
    mStorageUsesPrivateTheme( false ),
    mStorageUsesPrivateAggregation( false ),
    mStorageUsesPrivateSortOrder( false )
{
  Manager::registerWidget( this );

  setAutoFillBackground( true );
  setObjectName( "messagelistwidget" );

  QGridLayout * g = new QGridLayout( this );
  g->setMargin( 2 ); // use a smaller default
  g->setSpacing( 2 );

  mSearchEdit = new KLineEdit( this );
  mSearchEdit->setClickMessage( i18nc( "Search for messages.", "Search" ) );
  mSearchEdit->setObjectName( "quicksearch" );
  mSearchEdit->setClearButtonShown( true );

  connect( mSearchEdit, SIGNAL( textEdited( const QString & ) ),
           SLOT( searchEditTextEdited( const QString & ) ) );

  connect( mSearchEdit, SIGNAL( clearButtonClicked() ),
           SLOT( searchEditClearButtonClicked() ) );

  g->addWidget( mSearchEdit, 0, 0 );

  // The status filter button. Will be populated later, as populateStatusFilterCombo() is virtual
  mStatusFilterCombo = new KComboBox( this ) ;
  g->addWidget( mStatusFilterCombo, 0, 1 );

  // The "Open Full Search" button
  QToolButton * tb = new QToolButton( this );
  tb->setIcon( KIcon( "edit-find-mail" ) );
  tb->setText( i18n( "Open Full Search" ) );
  tb->setToolTip( tb->text() );
  g->addWidget( tb, 0, 2 );

  connect( tb, SIGNAL( clicked() ),
           this, SIGNAL( fullSearchRequest() ) );


  mView = new View( this );
  mView->setSortOrder( &mSortOrder );
  mView->setObjectName( "messagealistview" );
  g->addWidget( mView, 1, 0, 1, 6 );

  connect( mView->header(), SIGNAL( sectionClicked( int ) ),
           SLOT( slotViewHeaderSectionClicked( int ) ) );

  g->setRowStretch( 1, 1 );
  g->setColumnStretch( 0, 1 );

  mSearchEdit->setEnabled( false );
  mStatusFilterCombo->setEnabled( false );

  mSearchTimer = 0;
}

Widget::~Widget()
{
  mView->setStorageModel( 0 );

  Manager::unregisterWidget( this );

  delete mSearchTimer;
  delete mTheme;
  delete mAggregation;
  delete mFilter;
  delete mStorageModel;
}

void Widget::populateStatusFilterCombo()
{
  mStatusFilterCombo->clear();

  mStatusFilterCombo->addItem( i18n( "Any Status" ) );
  mStatusFilterCombo->setItemIcon( 0, SmallIcon("system-run") );
  mStatusFilterCombo->setItemData( 0, QVariant( static_cast< int >( 0 ) ) );

  mStatusFilterCombo->addItem( i18nc( "@action:inmenu Status of a message", "New" ) );
  mStatusFilterCombo->setItemIcon( 1, SmallIcon("mail-unread-new") );
  mStatusFilterCombo->setItemData( 1, QVariant( static_cast< int >( KPIM::MessageStatus::statusNew().toQInt32() ) ) );

  mStatusFilterCombo->addItem( i18nc( "@action:inmenu Status of a message", "Unread" ) );
  mStatusFilterCombo->setItemIcon( 2, SmallIcon("mail-unread") );
  mStatusFilterCombo->setItemData( 2, QVariant( static_cast< int >( KPIM::MessageStatus::statusUnread().toQInt32() | KPIM::MessageStatus::statusNew().toQInt32() ) ) );

  mStatusFilterCombo->addItem( i18nc( "@action:inmenu Status of a message", "Replied" ) );
  mStatusFilterCombo->setItemIcon( 3, SmallIcon("mail-replied") );
  mStatusFilterCombo->setItemData( 3, QVariant( static_cast< int >( KPIM::MessageStatus::statusReplied().toQInt32() ) ) );

  mStatusFilterCombo->addItem( i18nc( "@action:inmenu Status of a message", "Forwarded" ) );
  mStatusFilterCombo->setItemIcon( 4, SmallIcon("mail-forwarded") );
  mStatusFilterCombo->setItemData( 4, QVariant( static_cast< int >( KPIM::MessageStatus::statusForwarded().toQInt32() ) ) );

  mStatusFilterCombo->addItem( i18nc( "@action:inmenu Status of a message", "Important") );
  mStatusFilterCombo->setItemIcon( 5, SmallIcon("emblem-important") );
  mStatusFilterCombo->setItemData( 5, QVariant( static_cast< int >( KPIM::MessageStatus::statusImportant().toQInt32() ) ) );

  mStatusFilterCombo->addItem( i18n( "Action Item" ) );
  mStatusFilterCombo->setItemIcon( 6, SmallIcon("mail-task") );
  mStatusFilterCombo->setItemData( 6, QVariant( static_cast< int >( KPIM::MessageStatus::statusToAct().toQInt32() ) ) );

  mStatusFilterCombo->addItem( i18n( "Watched" ) );
  mStatusFilterCombo->setItemIcon( 7, SmallIcon("mail-thread-watch") );
  mStatusFilterCombo->setItemData( 7, QVariant( static_cast< int >( KPIM::MessageStatus::statusWatched().toQInt32() ) ) );

  mStatusFilterCombo->addItem( i18n( "Ignored" ) );
  mStatusFilterCombo->setItemIcon( 8, SmallIcon("mail-thread-ignored") );
  mStatusFilterCombo->setItemData( 8, QVariant( static_cast< int >( KPIM::MessageStatus::statusIgnored().toQInt32() ) ) );

  mStatusFilterCombo->addItem( i18n( "Has Attachment" ) );
  mStatusFilterCombo->setItemIcon( 9, SmallIcon("mail-attachment") );
  mStatusFilterCombo->setItemData( 9, QVariant( static_cast< int >( KPIM::MessageStatus::statusHasAttachment().toQInt32() ) ) );

  mStatusFilterCombo->addItem( i18n( "Spam" ) );
  mStatusFilterCombo->setItemIcon( 10, SmallIcon("mail-mark-junk") );
  mStatusFilterCombo->setItemData( 10, QVariant( static_cast< int >( KPIM::MessageStatus::statusSpam().toQInt32() ) ) );

  mStatusFilterCombo->addItem( i18n( "Ham" ) );
  mStatusFilterCombo->setItemIcon( 11, SmallIcon("mail-mark-notjunk") );
  mStatusFilterCombo->setItemData( 11, QVariant( static_cast< int >( KPIM::MessageStatus::statusHam().toQInt32() ) ) );

  mFirstTagInComboIndex = mStatusFilterCombo->count();
  fillMessageTagCombo( mStatusFilterCombo );

  disconnect( mStatusFilterCombo, SIGNAL( currentIndexChanged( int ) ),
             this, SLOT( statusSelected( int ) ) );
  connect( mStatusFilterCombo, SIGNAL( currentIndexChanged( int ) ),
           this, SLOT( statusSelected( int ) ) );
}

KPIM::MessageStatus Widget::currentFilterStatus() const
{
  if ( !mFilter )
    return KPIM::MessageStatus();

  KPIM::MessageStatus ret;
  ret.fromQInt32( mFilter->statusMask() );
  return ret;
}

QString Widget::currentFilterSearchString() const
{
  if ( !mFilter )
    return QString();

  return mFilter->searchString();
}

QString Widget::currentFilterTagId() const
{
  if ( !mFilter )
    return QString();

  return mFilter->tagId();
}

void Widget::setDefaultAggregationForStorageModel( const StorageModel * storageModel )
{
  const Aggregation * opt = Manager::instance()->aggregationForStorageModel( storageModel, &mStorageUsesPrivateAggregation );

  Q_ASSERT( opt );

  delete mAggregation;
  mAggregation = new Aggregation( *opt );

  mView->setAggregation( mAggregation );

  mLastAggregationId = opt->id();
}

void Widget::setDefaultThemeForStorageModel( const StorageModel * storageModel )
{
  const Theme * opt = Manager::instance()->themeForStorageModel( storageModel, &mStorageUsesPrivateTheme );

  Q_ASSERT( opt );

  delete mTheme;
  mTheme = new Theme( *opt );

  mView->setTheme( mTheme );

  mLastThemeId = opt->id();
}

void Widget::checkSortOrder( const StorageModel *storageModel )
{
  if ( storageModel && mAggregation && !mSortOrder.validForAggregation( mAggregation ) ) {
    kDebug() << "Could not restore sort order for folder" << storageModel->id();
    mSortOrder = SortOrder::defaultForAggregation( mAggregation, mSortOrder );

    // Change the global sort order if the sort order didn't fit the global aggregation.
    // Otherwise, if it is a per-folder aggregation, make the sort order per-folder too.
    if ( mStorageUsesPrivateAggregation )
      mStorageUsesPrivateSortOrder = true;
    if ( mStorageModel ) {
      Manager::instance()->saveSortOrderForStorageModel( storageModel, mSortOrder,
                                                         mStorageUsesPrivateSortOrder );
    }
    switchMessageSorting( mSortOrder.messageSorting(), mSortOrder.messageSortDirection(), -1 );
  }

}

void Widget::setDefaultSortOrderForStorageModel( const StorageModel * storageModel )
{
  // Load the sort order from config and update column headers
  mSortOrder = Manager::instance()->sortOrderForStorageModel( storageModel, &mStorageUsesPrivateSortOrder );
  switchMessageSorting( mSortOrder.messageSorting(), mSortOrder.messageSortDirection(), -1 );
  checkSortOrder( storageModel );
}

void Widget::setStorageModel( StorageModel * storageModel, PreSelectionMode preSelectionMode )
{
  if ( storageModel == mStorageModel )
    return; // nuthin to do here

  setDefaultAggregationForStorageModel( storageModel );
  setDefaultThemeForStorageModel( storageModel );
  setDefaultSortOrderForStorageModel( storageModel );

  if ( mSearchTimer )
  {
    mSearchTimer->stop();
    delete mSearchTimer;
    mSearchTimer = 0;
  }

  mSearchEdit->setText( QString() );

  if ( mFilter )
  {
    delete mFilter;
    mFilter = 0;

    mView->model()->setFilter( 0 );
  }

  StorageModel * oldModel = mStorageModel;

  mStorageModel = storageModel;

  mView->setStorageModel( mStorageModel, preSelectionMode );

  delete oldModel;

  mStatusFilterCombo->setEnabled( mStorageModel );
  mSearchEdit->setEnabled( mStorageModel );
}

void Widget::themeMenuAboutToShow()
{
  if ( !mStorageModel )
    return;

  KMenu * menu = dynamic_cast< KMenu * >( sender() );
  if ( !menu )
    return;

  menu->clear();

  menu->addTitle( i18n( "Theme" ) );

  QActionGroup * grp = new QActionGroup( menu );

  const QHash< QString, Theme * > & themes = Manager::instance()->themes();

  QAction * act;

  QList< const Theme * > sortedThemes;

  for ( QHash< QString, Theme * >::ConstIterator ci = themes.constBegin(); ci != themes.constEnd(); ++ci )
  {
    int idx = 0;
    int cnt = sortedThemes.count();
    while ( idx < cnt )
    {
      if ( sortedThemes.at( idx )->name() > ( *ci )->name() )
      {
        sortedThemes.insert( idx, *ci );
        break;
      }
      idx++;
    }

    if ( idx == cnt )
      sortedThemes.append( *ci );
  }

  for ( QList< const Theme * >::ConstIterator it = sortedThemes.constBegin(); it != sortedThemes.constEnd(); ++it )
  {
    act = menu->addAction( ( *it )->name() );
    act->setCheckable( true );
    grp->addAction( act );
    act->setChecked( mLastThemeId == ( *it )->id() );
    act->setData( QVariant( ( *it )->id() ) );
    connect( act, SIGNAL( triggered( bool ) ),
             SLOT( themeSelected( bool ) ) );
  }

  menu->addSeparator();

  act = menu->addAction( i18n( "Folder Always Uses This Theme" ) );
  act->setCheckable( true );
  act->setChecked( mStorageUsesPrivateTheme );
  connect( act, SIGNAL( triggered( bool ) ),
           SLOT( setPrivateThemeForStorage() ) );

  menu->addSeparator();

  act = menu->addAction( i18n( "Configure..." ) );
  connect( act, SIGNAL( triggered( bool ) ),
           SLOT( configureThemes() ) );
}

void Widget::setPrivateThemeForStorage()
{
  if ( !mStorageModel )
    return;

  Q_ASSERT( mTheme );

  mStorageUsesPrivateTheme = !mStorageUsesPrivateTheme;

  Manager::instance()->saveThemeForStorageModel( mStorageModel, mTheme->id(), mStorageUsesPrivateTheme );
}

void Widget::setPrivateSortOrderForStorage()
{
  if ( !mStorageModel )
    return;

  mStorageUsesPrivateSortOrder = !mStorageUsesPrivateSortOrder;

  Manager::instance()->saveSortOrderForStorageModel( mStorageModel, mSortOrder,
                                                     mStorageUsesPrivateSortOrder );
}

void Widget::configureThemes()
{
  // Show customization dialog
  Manager::instance()->showConfigureThemesDialog( this, mLastThemeId );
}

void Widget::themeSelected( bool )
{
  if ( !mStorageModel )
    return; // nuthin to do

  QAction * act = dynamic_cast< QAction * >( sender() );
  if ( !act )
    return;

  QVariant v = act->data();
  QString id = v.toString();

  if ( id.isEmpty() )
    return;

  const Theme * opt = Manager::instance()->theme( id );

  delete mTheme;
  mTheme = new Theme( *opt );

  mView->setTheme( mTheme );

  mLastThemeId = opt->id();

  //mStorageUsesPrivateTheme = false;

  Manager::instance()->saveThemeForStorageModel( mStorageModel, opt->id(), mStorageUsesPrivateTheme );

  mView->reload();

}

void Widget::aggregationMenuAboutToShow()
{
  KMenu * menu = dynamic_cast< KMenu * >( sender() );
  if ( !menu )
    return;

  menu->clear();

  menu->addTitle( i18n( "Aggregation" ) );

  QActionGroup * grp = new QActionGroup( menu );

  const QHash< QString, Aggregation * > & aggregations = Manager::instance()->aggregations();

  QAction * act;

  QList< const Aggregation * > sortedAggregations;

  for ( QHash< QString, Aggregation * >::ConstIterator ci = aggregations.constBegin(); ci != aggregations.constEnd(); ++ci )
  {
    int idx = 0;
    int cnt = sortedAggregations.count();
    while ( idx < cnt )
    {
      if ( sortedAggregations.at( idx )->name() > ( *ci )->name() )
      {
        sortedAggregations.insert( idx, *ci );
        break;
      }
      idx++;
    }

    if ( idx == cnt )
      sortedAggregations.append( *ci );
  }

  for ( QList< const Aggregation * >::ConstIterator it = sortedAggregations.constBegin(); it != sortedAggregations.constEnd(); ++it )
  {
    act = menu->addAction( ( *it )->name() );
    act->setCheckable( true );
    grp->addAction( act );
    act->setChecked( mLastAggregationId == ( *it )->id() );
    act->setData( QVariant( ( *it )->id() ) );
    connect( act, SIGNAL( triggered( bool ) ),
             SLOT( aggregationSelected( bool ) ) );
  }

  menu->addSeparator();

  act = menu->addAction( i18n( "Folder Always Uses This Aggregation" ) );
  act->setCheckable( true );
  act->setChecked( mStorageUsesPrivateAggregation );
  connect( act, SIGNAL( triggered( bool ) ),
           SLOT( setPrivateAggregationForStorage() ) );


  menu->addSeparator();

  act = menu->addAction( i18n( "Configure..." ) );
  act->setData( QVariant( QString() ) );
  connect( act, SIGNAL( triggered( bool ) ),
           SLOT( aggregationSelected( bool ) ) );
}

void Widget::setPrivateAggregationForStorage()
{
  if ( !mStorageModel )
    return;

  Q_ASSERT( mAggregation );

  mStorageUsesPrivateAggregation = !mStorageUsesPrivateAggregation;

  Manager::instance()->saveAggregationForStorageModel( mStorageModel, mAggregation->id(), mStorageUsesPrivateAggregation );
}


void Widget::aggregationSelected( bool )
{
  QAction * act = dynamic_cast< QAction * >( sender() );
  if ( !act )
    return;

  QVariant v = act->data();
  QString id = v.toString();

  if ( id.isEmpty() )
  {
    // Show customization dialog
    Manager::instance()->showConfigureAggregationsDialog( this, mLastAggregationId );
    return;
  }

  if ( !mStorageModel )
    return; // nuthin to do

  const Aggregation * opt = Manager::instance()->aggregation( id );

  delete mAggregation;
  mAggregation = new Aggregation( *opt );

  mView->setAggregation( mAggregation );

  mLastAggregationId = opt->id();

  //mStorageUsesPrivateAggregation = false;

  Manager::instance()->saveAggregationForStorageModel( mStorageModel, opt->id(), mStorageUsesPrivateAggregation );

  // The sort order might not be valid anymore for this aggregation
  checkSortOrder( mStorageModel );

  mView->reload();

}

void Widget::sortOrderMenuAboutToShow()
{
  if ( !mAggregation )
    return;

  KMenu * menu = dynamic_cast< KMenu * >( sender() );
  if ( !menu )
    return;

  menu->clear();

  menu->addTitle( i18n( "Message Sort Order" ) );

  QActionGroup * grp;
  QAction * act;
  QList< QPair< QString, int > > options;
  QList< QPair< QString, int > >::ConstIterator it;

  grp = new QActionGroup( menu );

  options = SortOrder::enumerateMessageSortingOptions( mAggregation->threading() );

  for ( it = options.constBegin(); it != options.constEnd(); ++it )
  {
    act = menu->addAction( ( *it ).first );
    act->setCheckable( true );
    grp->addAction( act );
    act->setChecked( mSortOrder.messageSorting() == ( *it ).second );
    act->setData( QVariant( ( *it ).second ) );
  }

  connect( grp, SIGNAL( triggered( QAction * ) ),
           SLOT( messageSortingSelected( QAction * ) ) );

  options = SortOrder::enumerateMessageSortDirectionOptions( mSortOrder.messageSorting() );

  if ( options.size() >= 2 )
  {
    menu->addTitle( i18n( "Message Sort Direction" ) );

    grp = new QActionGroup( menu );

    for ( it = options.constBegin(); it != options.constEnd(); ++it )
    {
      act = menu->addAction( ( *it ).first );
      act->setCheckable( true );
      grp->addAction( act );
      act->setChecked( mSortOrder.messageSortDirection() == ( *it ).second );
      act->setData( QVariant( ( *it ).second ) );
    }

    connect( grp, SIGNAL( triggered( QAction * ) ),
             SLOT( messageSortDirectionSelected( QAction * ) ) );
  }

  options = SortOrder::enumerateGroupSortingOptions( mAggregation->grouping() );

  if ( options.size() >= 2 )
  {
    menu->addTitle( i18n( "Group Sort Order" ) );

    grp = new QActionGroup( menu );

    for ( it = options.constBegin(); it != options.constEnd(); ++it )
    {
      act = menu->addAction( ( *it ).first );
      act->setCheckable( true );
      grp->addAction( act );
      act->setChecked( mSortOrder.groupSorting() == ( *it ).second );
      act->setData( QVariant( ( *it ).second ) );
    }

    connect( grp, SIGNAL( triggered( QAction * ) ),
             SLOT( groupSortingSelected( QAction * ) ) );
  }

  options = SortOrder::enumerateGroupSortDirectionOptions( mAggregation->grouping(),
                                                           mSortOrder.groupSorting() );

  if ( options.size() >= 2 )
  {
    menu->addTitle( i18n( "Group Sort Direction" ) );

    grp = new QActionGroup( menu );

    for ( it = options.constBegin(); it != options.constEnd(); ++it )
    {
      act = menu->addAction( ( *it ).first );
      act->setCheckable( true );
      grp->addAction( act );
      act->setChecked( mSortOrder.groupSortDirection() == ( *it ).second );
      act->setData( QVariant( ( *it ).second ) );
    }

    connect( grp, SIGNAL( triggered( QAction * ) ),
             SLOT( groupSortDirectionSelected( QAction * ) ) );
  }

  menu->addSeparator();
  act = menu->addAction( i18n( "Folder Always Uses This Sort Order" ) );
  act->setCheckable( true );
  act->setChecked( mStorageUsesPrivateSortOrder );
  connect( act, SIGNAL( triggered( bool ) ),
           SLOT( setPrivateSortOrderForStorage() ) );
}

void Widget::switchMessageSorting( SortOrder::MessageSorting messageSorting,
                                   SortOrder::SortDirection sortDirection,
                                   int logicalHeaderColumnIndex )
{
  mSortOrder.setMessageSorting( messageSorting );
  mSortOrder.setMessageSortDirection( sortDirection );

  // If the logicalHeaderColumnIndex was specified then we already know which
  // column we should set the sort indicator to. If it wasn't specified (it's -1)
  // then we need to find it out in the theme.

  if ( logicalHeaderColumnIndex == -1 )
  {
    // try to find the specified message sorting in the theme columns
    const QList< Theme::Column * > & columns = mTheme->columns();
    int idx = 0;

    // First try with a well defined message sorting.

    foreach( const Theme::Column* column, columns )
    {
      if ( !mView->header()->isSectionHidden( idx ) )
      {
        if ( column->messageSorting() == messageSorting )
        {
          // found a visible column with this message sorting
          logicalHeaderColumnIndex = idx;
          break;
        }
      }
      ++idx;
    }

    // if still not found, try again with a wider range
    if ( logicalHeaderColumnIndex == 1 )
    {
      idx = 0;
      foreach( const Theme::Column* column, columns )
      {
        if ( !mView->header()->isSectionHidden( idx ) )
        {
          if (
               (
                 ( column->messageSorting() == SortOrder::SortMessagesBySenderOrReceiver ) ||
                 ( column->messageSorting() == SortOrder::SortMessagesByReceiver ) ||
                 ( column->messageSorting() == SortOrder::SortMessagesBySender )
               ) && 
               (
                 ( messageSorting == SortOrder::SortMessagesBySenderOrReceiver ) ||
                 ( messageSorting == SortOrder::SortMessagesByReceiver ) ||
                 ( messageSorting == SortOrder::SortMessagesBySender )
               )
             )
          {
            // found a visible column with this message sorting
            logicalHeaderColumnIndex = idx;
            break;
          }
        }
        ++idx;
      }
    }
  }

  if ( logicalHeaderColumnIndex == -1 )
  {
    // not found: either not a column-based sorting or the related column is hidden
    mView->header()->setSortIndicatorShown( false );
    return;
  }

  mView->header()->setSortIndicatorShown( true );

  if ( sortDirection == SortOrder::Ascending )
    mView->header()->setSortIndicator( logicalHeaderColumnIndex, Qt::AscendingOrder );
  else
    mView->header()->setSortIndicator( logicalHeaderColumnIndex, Qt::DescendingOrder );
}

void Widget::messageSortingSelected( QAction *action )
{
  if ( !mAggregation )
    return;
  if ( !action )
    return;

  bool ok;
  SortOrder::MessageSorting ord = static_cast< SortOrder::MessageSorting >( action->data().toInt( &ok ) );

  if ( !ok )
    return;

  switchMessageSorting( ord, mSortOrder.messageSortDirection(), -1 );
  Manager::instance()->saveSortOrderForStorageModel( mStorageModel, mSortOrder,
                                                     mStorageUsesPrivateSortOrder );

  mView->reload();

}

void Widget::messageSortDirectionSelected( QAction *action )
{
  if ( !mAggregation )
    return;
  if ( !action )
    return;

  bool ok;
  SortOrder::SortDirection ord = static_cast< SortOrder::SortDirection >( action->data().toInt( &ok ) );

  if ( !ok )
    return;

  switchMessageSorting( mSortOrder.messageSorting(), ord, -1 );
  Manager::instance()->saveSortOrderForStorageModel( mStorageModel, mSortOrder,
                                                     mStorageUsesPrivateSortOrder );

  mView->reload();

}

void Widget::groupSortingSelected( QAction *action )
{
  if ( !mAggregation )
    return;
  if ( !action )
    return;

  bool ok;
  SortOrder::GroupSorting ord = static_cast< SortOrder::GroupSorting >( action->data().toInt( &ok ) );

  if ( !ok )
    return;

  mSortOrder.setGroupSorting( ord );
  Manager::instance()->saveSortOrderForStorageModel( mStorageModel, mSortOrder,
                                                     mStorageUsesPrivateSortOrder );

  mView->reload();

}

void Widget::groupSortDirectionSelected( QAction *action )
{
  if ( !mAggregation )
    return;
  if ( !action )
    return;

  bool ok;
  SortOrder::SortDirection ord = static_cast< SortOrder::SortDirection >( action->data().toInt( &ok ) );

  if ( !ok )
    return;

  mSortOrder.setGroupSortDirection( ord );
  Manager::instance()->saveSortOrderForStorageModel( mStorageModel, mSortOrder,
                                                     mStorageUsesPrivateSortOrder );

  mView->reload();

}

void Widget::slotViewHeaderSectionClicked( int logicalIndex )
{
  if ( !mTheme )
    return;

  if ( !mAggregation )
    return;

  if ( logicalIndex >= mTheme->columns().count() )
    return;

  const Theme::Column * column = mTheme->column( logicalIndex );
  if ( !column )
    return; // should never happen...

  if ( column->messageSorting() == SortOrder::NoMessageSorting )
    return; // this is a null op.


  if ( mSortOrder.messageSorting() == column->messageSorting() )
  {
    // switch sort direction
    if ( mSortOrder.messageSortDirection() == SortOrder::Ascending )
      switchMessageSorting( mSortOrder.messageSorting(), SortOrder::Descending, logicalIndex );
    else
      switchMessageSorting( mSortOrder.messageSorting(), SortOrder::Ascending, logicalIndex );
  } else {
    // keep sort direction but switch sort order
    switchMessageSorting( column->messageSorting(), mSortOrder.messageSortDirection(), logicalIndex );
  }
  Manager::instance()->saveSortOrderForStorageModel( mStorageModel, mSortOrder,
                                                     mStorageUsesPrivateSortOrder );

  mView->reload();

}

void Widget::themesChanged()
{
  setDefaultThemeForStorageModel( mStorageModel );

  mView->reload();
}

void Widget::aggregationsChanged()
{
  setDefaultAggregationForStorageModel( mStorageModel );
  checkSortOrder( mStorageModel );

  mView->reload();
}

void Widget::fillMessageTagCombo( KComboBox* /*combo*/ )
{
  // nothing here: must be overridden in derived classes
}

void Widget::tagIdSelected( QVariant data )
{
  QString tagId = data.toString();

  // Here we arbitrairly set the status to 0, though we *could* allow filtering
  // by status AND tag...

  if ( mFilter )
    mFilter->setStatusMask( 0 );

  if ( tagId.isEmpty() )
  {
    if ( mFilter )
    {
      if ( mFilter->isEmpty() )
      {
        delete mFilter;
        mFilter = 0;
      }
    }
  } else {
    if ( !mFilter )
      mFilter = new Filter();
    mFilter->setTagId( tagId );
  }

  mView->model()->setFilter( mFilter );
}

void Widget::statusSelected( int index )
{
  if ( index >= mFirstTagInComboIndex ) {
    tagIdSelected( mStatusFilterCombo->itemData( index ) );
    return;
  }

  bool ok;
  qint32 additionalStatusMask = static_cast< qint32 >( mStatusFilterCombo->itemData( index ).toInt( &ok ) );
  if ( !ok )
    return;

  // Here we override the whole status at once.
  // This is a restriction but at least a couple of people
  // are telling me that this way it's more usable. Two are more than me
  // so here we go :)
  qint32 statusMask = 0; //mFilter ? mFilter->statusMask() : 0; <-- this would "or" with the existing mask instead

  // We also arbitrairly set tagId to an empty string, though we *could* allow filtering
  // by status AND tag...
  if ( mFilter )
    mFilter->setTagId( QString() );

  if ( additionalStatusMask == 0)
  {
    if ( mFilter )
    {
      mFilter->setStatusMask( 0 );
      if ( mFilter->isEmpty() )
      {
        delete mFilter;
        mFilter = 0;
      }
    }
  } else {
    if ( statusMask & additionalStatusMask )
    {
      // already have this status bit (this actually never happens because of the override above)
      if ( mFilter )
      {
        mFilter->setStatusMask( statusMask & ~additionalStatusMask );
        if ( mFilter->isEmpty() )
        {
          delete mFilter;
          mFilter = 0;
        }
      } // else nothing to remove (but something weird happened in the code above...)
    } else {
      // don't have this status bit
      if ( !mFilter )
        mFilter = new Filter();
      mFilter->setStatusMask( statusMask | additionalStatusMask );
    }
  }

  mView->model()->setFilter( mFilter );
}

void Widget::searchEditTextEdited( const QString & )
{
  // This slot is called whenever the user edits the search QLineEdit.
  // Since the user is likely to type more than one character
  // so we start the real search after a short delay in order to catch
  // multiple textEdited() signals.

  if ( !mSearchTimer )
  {
    mSearchTimer = new QTimer( this );
    connect( mSearchTimer, SIGNAL( timeout() ),
             SLOT( searchTimerFired() ) );
  } else {
    mSearchTimer->stop(); // eventually
  }

  mSearchTimer->setSingleShot( true );
  mSearchTimer->start( 1000 );

}

void Widget::searchTimerFired()
{
  // A search is pending.

  if ( mSearchTimer )
    mSearchTimer->stop();

  if ( !mFilter )
    mFilter = new Filter();

  QString text = mSearchEdit->text();

  mFilter->setSearchString( text );
  if ( mFilter->isEmpty() )
  {
    delete mFilter;
    mFilter = 0;
  }

  mView->model()->setFilter( mFilter );
}

void Widget::searchEditClearButtonClicked()
{
  if ( !mFilter )
    return;

  delete mFilter;
  mFilter = 0;

  mView->model()->setFilter( mFilter );
}


void Widget::viewMessageSelected( MessageItem * )
{
}

void Widget::viewMessageActivated( MessageItem * )
{
}

void Widget::viewSelectionChanged()
{
}

void Widget::viewMessageListContextPopupRequest( const QList< MessageItem * > &, const QPoint & )
{
}

void Widget::viewGroupHeaderContextPopupRequest( GroupHeaderItem *, const QPoint & )
{
}

void Widget::viewDragEnterEvent( QDragEnterEvent * )
{
}

void Widget::viewDragMoveEvent( QDragMoveEvent * )
{
}

void Widget::viewDropEvent( QDropEvent * )
{
}

void Widget::viewStartDragRequest()
{
}

void Widget::viewJobBatchStarted()
{
}

void Widget::viewJobBatchTerminated()
{
}

void Widget::viewMessageStatusChangeRequest( MessageItem *msg, const KPIM::MessageStatus &set, const KPIM::MessageStatus &clear )
{
  Q_UNUSED( msg );
  Q_UNUSED( set );
  Q_UNUSED( clear );
}

} // namespace Core

} // namespace MessageListView

} // namespace KMail
