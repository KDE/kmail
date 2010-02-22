/* -*- mode: C++; c-file-style: "gnu" -*-
  This file is part of KMail, the KDE mail client.
  Copyright (c) 2009, 2010 Montel Laurent <montel@kde.org>

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

#include "folderselectiontreeview.h"
#include "foldertreeview.h"
#include <QSortFilterProxyModel>
#include <QHBoxLayout>

#include <akonadi/attributefactory.h>
#include <akonadi/entitytreeview.h>
#include <akonadi/changerecorder.h>
#include <akonadi/session.h>
#include <akonadi/entitytreemodel.h>
#include <akonadi/favoritecollectionsmodel.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/entitymimetypefiltermodel.h>
#include <akonadi/collection.h>
#include <akonadi/statisticsproxymodel.h>
#include <akonadi_next/quotacolorproxymodel.h>
#include <akonadi_next/recursivecollectionfilterproxymodel.h>

#include <akonadi/krecursivefilterproxymodel.h>

#include <KMime/Message>

#include "imapaclattribute.h"

#include "readablecollectionproxymodel.h"

#include "globalsettings.h"
#include "kmkernel.h"

class FolderSelectionTreeView::FolderSelectionTreeViewPrivate
{
public:
  FolderSelectionTreeViewPrivate()
    :filterModel( 0 ),
     collectionFolderView( 0 ),
     entityModel( 0 ),
     quotaModel( 0 ),
     readableproxy( 0 )
  {
  }
  Akonadi::StatisticsProxyModel *filterModel;
  FolderTreeView *collectionFolderView;
  Akonadi::EntityTreeModel *entityModel;
  Akonadi::QuotaColorProxyModel *quotaModel;
  ReadableCollectionProxyModel *readableproxy;
  KRecursiveFilterProxyModel *filterTreeViewModel;
  KLineEdit *filterFolderLineEdit;
  QLabel *label;
};


FolderSelectionTreeView::FolderSelectionTreeView( QWidget *parent, KXMLGUIClient *xmlGuiClient, TreeViewOptions options )
  : QWidget( parent ), d( new FolderSelectionTreeViewPrivate() )
{
  Akonadi::AttributeFactory::registerAttribute<Akonadi::ImapAclAttribute>();

  d->collectionFolderView = new FolderTreeView( xmlGuiClient, this, options & ShowUnreadCount );

  QVBoxLayout *lay = new QVBoxLayout( this );
  lay->setMargin( 0 );
  Akonadi::Session *session = new Akonadi::Session( "KMail Session", this );

  KMKernel::self()->monitor()->setSession( session );

  d->entityModel = new Akonadi::EntityTreeModel( KMKernel::self()->monitor(), this );
  d->entityModel->setItemPopulationStrategy( Akonadi::EntityTreeModel::LazyPopulation );

  d->label = new QLabel( i18n("You can start typing to filter the list of folders."), this);
  lay->addWidget( d->label );

  d->filterFolderLineEdit = new KLineEdit( this );
  d->filterFolderLineEdit->setClearButtonShown( true );
  d->filterFolderLineEdit->setClickMessage( i18nc( "@info/plain Displayed grayed-out inside the "
                                                   "textbox, verb to search", "Search" ) );
  lay->addWidget( d->filterFolderLineEdit );

  Akonadi::EntityMimeTypeFilterModel *collectionModel = new Akonadi::EntityMimeTypeFilterModel( this );
  collectionModel->setSourceModel( d->entityModel );
  collectionModel->addMimeTypeInclusionFilter( Akonadi::Collection::mimeType() );
  collectionModel->setHeaderGroup( Akonadi::EntityTreeModel::CollectionTreeHeaders );
  collectionModel->setDynamicSortFilter( true );
  collectionModel->setSortCaseSensitivity( Qt::CaseInsensitive );

  Akonadi::RecursiveCollectionFilterProxyModel *recurfilter = new Akonadi::RecursiveCollectionFilterProxyModel( this );
  recurfilter->addContentMimeTypeInclusionFilter( KMime::Message::mimeType() );
  recurfilter->setSourceModel( collectionModel );

  // ... with statistics...
  d->filterModel = new Akonadi::StatisticsProxyModel( this );
  d->filterModel->setSourceModel( recurfilter );

  d->quotaModel = new Akonadi::QuotaColorProxyModel( this );
  d->quotaModel->setSourceModel( d->filterModel );

  d->readableproxy = new ReadableCollectionProxyModel( this );
  d->readableproxy->setSourceModel( d->quotaModel );

  connect( d->collectionFolderView, SIGNAL(changeTooltipsPolicy( FolderSelectionTreeView::ToolTipDisplayPolicy ) ), this, SLOT( slotChangeTooltipsPolicy( FolderSelectionTreeView::ToolTipDisplayPolicy ) ) );

  d->collectionFolderView->setSelectionMode( QAbstractItemView::SingleSelection );
  // Use the model

  //Filter tree view.
  d->filterTreeViewModel = new KRecursiveFilterProxyModel( this );
  d->filterTreeViewModel->setDynamicSortFilter( true );
  d->filterTreeViewModel->setSourceModel( d->readableproxy );
  d->filterTreeViewModel->setFilterCaseSensitivity( Qt::CaseInsensitive );
  d->collectionFolderView->setModel( d->filterTreeViewModel );

  lay->addWidget( d->collectionFolderView );
  if ( ( options & UseLineEditForFiltering ) ) {
    connect( d->filterFolderLineEdit, SIGNAL( textChanged(QString) ), d->filterTreeViewModel, SLOT( setFilterFixedString(QString) ) );
    d->label->hide();
  } else {
    d->filterFolderLineEdit->hide();
  }

  readConfig();
}


FolderSelectionTreeView::~FolderSelectionTreeView()
{
  delete d;
}

void FolderSelectionTreeView::disableContextMenuAndExtraColumn()
{
  d->collectionFolderView->disableContextMenuAndExtraColumn();
}

void FolderSelectionTreeView::selectCollectionFolder( const Akonadi::Collection & col )
{
  const QModelIndex idx = d->collectionFolderView->model()->index( 0, 0, QModelIndex() );
  const QModelIndexList rows = d->collectionFolderView->model()->match( idx, Akonadi::EntityTreeModel::CollectionIdRole, col.id(), -1, Qt::MatchRecursive | Qt::MatchExactly );

  if ( rows.size() < 1 )
    return;
  const QModelIndex colIndex = rows.first();
  d->collectionFolderView->selectionModel()->select(colIndex, QItemSelectionModel::SelectCurrent);
  d->collectionFolderView->setExpanded( colIndex, true );
  d->collectionFolderView->scrollTo( colIndex );
}

void FolderSelectionTreeView::setSelectionMode( QAbstractItemView::SelectionMode mode )
{
  d->collectionFolderView->setSelectionMode( mode );
}

QAbstractItemView::SelectionMode FolderSelectionTreeView::selectionMode() const
{
  return d->collectionFolderView->selectionMode();
}


QItemSelectionModel * FolderSelectionTreeView::selectionModel () const
{
  return d->collectionFolderView->selectionModel();
}

QModelIndex FolderSelectionTreeView::currentIndex() const
{
  return d->collectionFolderView->currentIndex();
}


Akonadi::Collection FolderSelectionTreeView::selectedCollection() const
{
  if ( d->collectionFolderView->selectionMode() == QAbstractItemView::SingleSelection ) {
    const QModelIndex selectedIndex = d->collectionFolderView->currentIndex();
    QModelIndex index = selectedIndex.sibling( selectedIndex.row(), 0 );
    if ( index.isValid() )
      return index.model()->data( index, Akonadi::EntityTreeModel::CollectionRole ).value<Akonadi::Collection>();
  }

  return Akonadi::Collection();
}

Akonadi::Collection::List FolderSelectionTreeView::selectedCollections() const
{
  Akonadi::Collection::List collections;
  const QItemSelectionModel *selectionModel = d->collectionFolderView->selectionModel();
  const QModelIndexList selectedIndexes = selectionModel->selectedIndexes();
  foreach ( const QModelIndex &index, selectedIndexes ) {
    if ( index.isValid() ) {
      const Akonadi::Collection collection = index.model()->data( index, Akonadi::EntityTreeModel::CollectionRole ).value<Akonadi::Collection>();
      if ( collection.isValid() )
        collections.append( collection );
    }
  }

  return collections;
}

FolderTreeView* FolderSelectionTreeView::folderTreeView()
{
  return d->collectionFolderView;
}

Akonadi::EntityTreeModel *FolderSelectionTreeView::entityModel()
{
  return d->entityModel;
}

void FolderSelectionTreeView::readConfig()
{
  // Custom/System font support
  KConfigGroup fontConfig( KMKernel::config(), "Fonts" );
  if (!fontConfig.readEntry( "defaultFonts", true ) )
    setFont( fontConfig.readEntry("folder-font", KGlobalSettings::generalFont() ) );
  else
    setFont( KGlobalSettings::generalFont() );

  KConfigGroup mainFolderView( KMKernel::config(), "MainFolderView" );
  const int checkedFolderToolTipsPolicy = mainFolderView.readEntry( "ToolTipDisplayPolicy", 0 );
  changeToolTipsPolicyConfig( ( ToolTipDisplayPolicy )checkedFolderToolTipsPolicy );

  d->collectionFolderView->setShowDropActionMenu( GlobalSettings::self()->showPopupAfterDnD() );
  readQuotaConfig();
}

void FolderSelectionTreeView::slotChangeTooltipsPolicy( FolderSelectionTreeView::ToolTipDisplayPolicy policy)
{
  changeToolTipsPolicyConfig( policy );
}


void FolderSelectionTreeView::changeToolTipsPolicyConfig( ToolTipDisplayPolicy policy )
{
  switch( policy ){
  case DisplayAlways:
  case DisplayWhenTextElided: //Need to implement in the future
    d->filterModel->setToolTipEnabled( true );
    break;
  case DisplayNever:
    d->filterModel->setToolTipEnabled( false );
  }
  d->collectionFolderView->setTooltipsPolicy( policy );
}

void FolderSelectionTreeView::quotaWarningParameters( const QColor &color, qreal threshold )
{
  d->quotaModel->setWarningThreshold( threshold );
  d->quotaModel->setWarningColor( color );
}

void FolderSelectionTreeView::readQuotaConfig()
{
  QColor quotaColor;
  qreal threshold = 100;
  if ( !GlobalSettings::self()->useDefaultColors() ) {
    KConfigGroup readerConfig( KMKernel::config(), "Reader" );
    quotaColor = readerConfig.readEntry( "CloseToQuotaColor", quotaColor  );
    threshold = GlobalSettings::closeToQuotaThreshold();
  }
  quotaWarningParameters( quotaColor, threshold );
}

Akonadi::StatisticsProxyModel * FolderSelectionTreeView::statisticsProxyModel()
{
  return d->filterModel;
}

ReadableCollectionProxyModel *FolderSelectionTreeView::readableCollectionProxyModel()
{
  return d->readableproxy;
}

KLineEdit *FolderSelectionTreeView::filterFolderLineEdit()
{
  return d->filterFolderLineEdit;
}

void FolderSelectionTreeView::applyFilter( const QString &filter )
{

  d->label->setText( filter.isEmpty() ? i18n("You can start typing to filter the list of folders.") : i18n( "Path: (%1)", filter ) );
  d->filterTreeViewModel->setFilterFixedString( filter );
  d->collectionFolderView->expandAll();
}

#include "folderselectiontreeview.moc"
