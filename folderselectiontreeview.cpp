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

#include <akonadi/statisticstooltipproxymodel.h>
#include <akonadi/entitytreeview.h>
#include <akonadi/changerecorder.h>
#include <akonadi/session.h>
#include <akonadi/entitytreemodel.h>
#include <akonadi/favoritecollectionsmodel.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/entityfilterproxymodel.h>
#include <akonadi/collection.h>
#include <akonadi/statisticsproxymodel.h>

class FolderSelectionTreeView::FolderSelectionTreeViewPrivate
{
public:
  FolderSelectionTreeViewPrivate()
    :filterModel( 0 ), collectionFolderView( 0 ), entityModel( 0 ), monitor( 0 )
  {
  }
  QSortFilterProxyModel *filterModel;
  FolderTreeView *collectionFolderView;
  Akonadi::EntityTreeModel *entityModel;
  Akonadi::ChangeRecorder *monitor;
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



  Akonadi::EntityFilterProxyModel *collectionModel = new Akonadi::EntityFilterProxyModel( this );
  collectionModel->setSourceModel( d->entityModel );
  collectionModel->addMimeTypeInclusionFilter( Akonadi::Collection::mimeType() );
  collectionModel->setHeaderSet( Akonadi::EntityTreeModel::CollectionTreeHeaders );

  // ... with statistics...
  Akonadi::StatisticsToolTipProxyModel *statisticsProxyModel = new Akonadi::StatisticsToolTipProxyModel( this );
  statisticsProxyModel->setSourceModel( collectionModel );


  d->filterModel = new Akonadi::StatisticsProxyModel(this);
  d->filterModel->setSourceModel( statisticsProxyModel );
  d->filterModel->setDynamicSortFilter( true );
  d->filterModel->setSortCaseSensitivity( Qt::CaseInsensitive );

  d->collectionFolderView = new FolderTreeView( xmlGuiClient, this );

  d->collectionFolderView->setSelectionMode( QAbstractItemView::SingleSelection );
  // Use the model
  d->collectionFolderView->setModel( d->filterModel );
  d->collectionFolderView->expandAll();
  lay->addWidget( d->collectionFolderView );

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
    const QModelIndex index = d->collectionFolderView->currentIndex();
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

#include "folderselectiontreeview.moc"
