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

class FolderSelectionTreeView::FolderSelectionTreeViewPrivate
{
public:
  FolderSelectionTreeViewPrivate()
    :filterModel( 0 ), collectionFolderView( 0 ), entityModel( 0 )
  {
  }
  QSortFilterProxyModel *filterModel;
  Akonadi::EntityTreeView *collectionFolderView;
  Akonadi::EntityTreeModel *entityModel;
};



FolderSelectionTreeView::FolderSelectionTreeView( QWidget *parent )
  : QWidget( parent ), d( new FolderSelectionTreeViewPrivate() )
{
  QHBoxLayout *lay = new QHBoxLayout( this );
  lay->setMargin( 0 );
  Akonadi::Session *session = new Akonadi::Session( "Folder Selection TreeView", this );

  // monitor collection changes
  Akonadi::ChangeRecorder *monitor = new Akonadi::ChangeRecorder( this );
  monitor->setCollectionMonitored( Akonadi::Collection::root() );
  monitor->fetchCollection( true );
  monitor->setAllMonitored( true );
  monitor->setMimeTypeMonitored( "message/rfc822" );
  // TODO: Only fetch the envelope etc if possible.
  monitor->itemFetchScope().fetchFullPayload(true);
  d->entityModel = new Akonadi::EntityTreeModel( session, monitor, this );

  Akonadi::EntityFilterProxyModel *collectionModel = new Akonadi::EntityFilterProxyModel( this );
  collectionModel->setSourceModel( d->entityModel );
  collectionModel->addMimeTypeInclusionFilter( Akonadi::Collection::mimeType() );
  collectionModel->setHeaderSet( Akonadi::EntityTreeModel::CollectionTreeHeaders );

  // ... with statistics...
  Akonadi::StatisticsToolTipProxyModel *statisticsProxyModel = new Akonadi::StatisticsToolTipProxyModel( this );
  statisticsProxyModel->setSourceModel( collectionModel );


  d->filterModel = new QSortFilterProxyModel( this );
  d->filterModel->setDynamicSortFilter( true );
  d->filterModel->setSortCaseSensitivity( Qt::CaseInsensitive );
  d->filterModel->setSourceModel( statisticsProxyModel );

  d->collectionFolderView = new Akonadi::EntityTreeView( 0, this );

  d->collectionFolderView->setSelectionMode( QAbstractItemView::SingleSelection );
  // Use the model
  d->collectionFolderView->setModel( d->filterModel );

  lay->addWidget( d->collectionFolderView );

}


FolderSelectionTreeView::~FolderSelectionTreeView()
{
  delete d;
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

#include "folderselectiontreeview.moc"
