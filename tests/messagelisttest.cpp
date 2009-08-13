/*
    Copyright (c) 2009 Kevin Ottens <ervin@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

    As a special exception, permission is given to link this program
    with any edition of Qt, and distribute the resulting executable,
    without including the source code for Qt in the source distribution.
*/

#include "messagelisttest.h"

#include "akonadi_next/entitytreeview.h"
#include <akonadi/entityfilterproxymodel.h>
#include <akonadi/entitytreemodel.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/monitor.h>
#include <akonadi/selectionproxymodel.h>
#include <akonadi/session.h>
#include <akonadi/statisticstooltipproxymodel.h>

#include <KDE/KAboutData>
#include <KDE/KApplication>
#include <KDE/KCmdLineArgs>

#include <QtGui/QLayout>
#include <QtGui/QSplitter>
#include <QtGui/QSortFilterProxyModel>

#include "messagelistview/akonadi/ak_storagemodel.h"
#include "messagelistview/akonadi/ak_widget.h"

using namespace Akonadi;

MainWidget::MainWidget( QWidget *parent )
  : QWidget( parent )
{
  setLayout( new QVBoxLayout( this ) );

  QSplitter *splitter = new QSplitter( Qt::Horizontal, this );
  layout()->addWidget( splitter );

  EntityTreeView *collectionView = new EntityTreeView( 0, this );
  collectionView->setSelectionMode( QAbstractItemView::ExtendedSelection );
  splitter->addWidget( collectionView );

  MessageListView::Widget *messageView = new MessageListView::Widget( this );
  splitter->addWidget( messageView );


  Session *session = new Session( "AkonadiConsole Browser Widget", this );

  Monitor *monitor = new Monitor( this );
  monitor->setCollectionMonitored( Collection::root() );
  monitor->fetchCollection( true );
  monitor->setAllMonitored( true );
  monitor->itemFetchScope().fetchFullPayload(true);

  EntityTreeModel *entityModel = new EntityTreeModel( session, monitor, this );
  entityModel->setItemPopulationStrategy( EntityTreeModel::LazyPopulation );

  EntityFilterProxyModel *collectionFilter = new EntityFilterProxyModel( this );
  collectionFilter->setSourceModel( entityModel );
  collectionFilter->addMimeTypeInclusionFilter( Collection::mimeType() );
  collectionFilter->setHeaderSet( EntityTreeModel::CollectionTreeHeaders );

  StatisticsToolTipProxyModel *statisticsProxyModel = new StatisticsToolTipProxyModel( this );
  statisticsProxyModel->setSourceModel( collectionFilter );

  QSortFilterProxyModel *sortModel = new QSortFilterProxyModel( this );
  sortModel->setDynamicSortFilter( true );
  sortModel->setSortCaseSensitivity( Qt::CaseInsensitive );
  sortModel->setSourceModel( statisticsProxyModel );

  collectionView->setModel( sortModel );


  MessageListView::StorageModel *messageModel = new MessageListView::StorageModel( entityModel, collectionView->selectionModel(), this );
  messageView->setStorageModel( messageModel );
}

int main( int argc, char **argv )
{
  KAboutData aboutData( "messagelisttest", 0,
   ki18n("Test Message List View"), "0.1" );
  KCmdLineArgs::init( argc, argv, &aboutData );

  KApplication app;

  MainWidget w;
  w.show();
  return app.exec();
}

#include "messagelisttest.moc"
