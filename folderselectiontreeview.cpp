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

#include "folderselectiontreeview.h"
#include "foldertreeview.h"
#include <QSortFilterProxyModel>
#include <QHBoxLayout>

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
     monitor( 0 ),
     quotaModel( 0 ),
     readableproxy( 0 )
  {
  }
  FolderTreeView *collectionFolderView;
  Akonadi::EntityTreeModel *entityModel;
  Akonadi::ChangeRecorder *monitor;
  Akonadi::QuotaColorProxyModel *quotaModel;
  Akonadi::StatisticsProxyModel *filterModel;
  ReadableCollectionProxyModel *readableproxy;
};



FolderSelectionTreeView::FolderSelectionTreeView( QWidget *parent, KXMLGUIClient *xmlGuiClient )
  : QWidget( parent ), d( new FolderSelectionTreeViewPrivate() )
{
  QHBoxLayout *lay = new QHBoxLayout( this );
  lay->setMargin( 0 );
  Akonadi::Session *session = new Akonadi::Session( "KMail Session", this );

  // monitor collection changes
  d->monitor = new Akonadi::ChangeRecorder( this );
  d->monitor->setCollectionMonitored( Akonadi::Collection::root() );
  d->monitor->fetchCollection( true );
  d->monitor->setAllMonitored( true );
  d->monitor->setMimeTypeMonitored( "message/rfc822" );

  // TODO: Only fetch the envelope etc if possible.
  d->monitor->itemFetchScope().fetchFullPayload(true);
  d->entityModel = new Akonadi::EntityTreeModel( session, d->monitor, this );



  Akonadi::EntityMimeTypeFilterModel *collectionModel = new Akonadi::EntityMimeTypeFilterModel( this );
  collectionModel->setSourceModel( d->entityModel );
  collectionModel->addMimeTypeInclusionFilter( Akonadi::Collection::mimeType() );
  collectionModel->setHeaderGroup( Akonadi::EntityTreeModel::CollectionTreeHeaders );

  // ... with statistics...
  d->filterModel = new Akonadi::StatisticsProxyModel(this);
  d->filterModel->setSourceModel( collectionModel );
  d->filterModel->setDynamicSortFilter( true );
  d->filterModel->setSortCaseSensitivity( Qt::CaseInsensitive );


  d->quotaModel = new Akonadi::QuotaColorProxyModel( this );
  d->quotaModel->setSourceModel( d->filterModel );

  d->readableproxy = new ReadableCollectionProxyModel( this );
  d->readableproxy->setSourceModel( d->quotaModel );

  d->collectionFolderView = new FolderTreeView( xmlGuiClient, this );

  d->collectionFolderView->setSelectionMode( QAbstractItemView::SingleSelection );
  // Use the model
  d->collectionFolderView->setModel( d->readableproxy );
  d->collectionFolderView->expandAll();
  lay->addWidget( d->collectionFolderView );

  readConfig();
}


FolderSelectionTreeView::~FolderSelectionTreeView()
{
  delete d;
}


Akonadi::ChangeRecorder * FolderSelectionTreeView::monitorFolders()
{
  return d->monitor;
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
  switch( checkedFolderToolTipsPolicy ){
  case DisplayAlways:
  case DisplayWhenTextElided: //Need to implement in the future
    d->filterModel->setToolTipEnabled( true );
    break;
  case DisplayNever:
    d->filterModel->setToolTipEnabled( false );
  }

  readQuotaConfig();
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

#include "folderselectiontreeview.moc"
