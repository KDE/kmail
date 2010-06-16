/*
    Copyright (c) 2009, 2010 Laurent Montel <montel@kde.org>


    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "readablecollectionproxymodel.h"
#include "foldercollection.h"

#include <akonadi/collection.h>
#include <akonadi/entitytreemodel.h>
#include <kdebug.h>

#include <akonadi_next/krecursivefilterproxymodel.h>

#include <QtGui/QApplication>
#include <QtGui/QPalette>

class ReadableCollectionProxyModel::Private
{
public:
  Private()
    : enableCheck( false ),
      hideVirtualFolder( false ),
      hideSpecificFolder( false )
    {
    }
  bool enableCheck;
  bool hideVirtualFolder;
  bool hideSpecificFolder;
};

ReadableCollectionProxyModel::ReadableCollectionProxyModel( QObject *parent )
  : Akonadi::EntityRightsFilterModel( parent ),
    d( new Private )
{
  setDynamicSortFilter( true );
}

ReadableCollectionProxyModel::~ReadableCollectionProxyModel()
{
  delete d;
}


Qt::ItemFlags ReadableCollectionProxyModel::flags( const QModelIndex & index ) const
{
  if ( d->enableCheck )
  {
    const QModelIndex sourceIndex = mapToSource( index.sibling( index.row(), 0 ) );
    const Akonadi::Collection collection = sourceModel()->data( sourceIndex, Akonadi::EntityTreeModel::CollectionRole ).value<Akonadi::Collection>();

    if ( ! ( collection.rights() & Akonadi::Collection::CanCreateItem ) ) {
      return Future::KRecursiveFilterProxyModel::flags( index ) & ~(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    }
    return Akonadi::EntityRightsFilterModel::flags( index );
  }

  return QSortFilterProxyModel::flags( index );
}

void ReadableCollectionProxyModel::setEnabledCheck( bool enable )
{
  d->enableCheck = enable;
}

bool ReadableCollectionProxyModel::enabledCheck() const
{
  return d->enableCheck;
}

void ReadableCollectionProxyModel::setHideVirtualFolder( bool exclude )
{
  d->hideVirtualFolder = exclude;
}

bool ReadableCollectionProxyModel::hideVirtualFolder() const
{
  return d->hideVirtualFolder;
}

void ReadableCollectionProxyModel::setHideSpecificFolder( bool hide )
{
  d->hideSpecificFolder = hide;
}

bool ReadableCollectionProxyModel::hideSpecificFolder() const
{
  return d->hideSpecificFolder;
}

bool ReadableCollectionProxyModel::acceptRow( int sourceRow, const QModelIndex &sourceParent) const
{
  const QModelIndex modelIndex = sourceModel()->index( sourceRow, 0, sourceParent );

  const Akonadi::Collection collection = sourceModel()->data( modelIndex, Akonadi::EntityTreeModel::CollectionRole ).value<Akonadi::Collection>();
  if ( d->hideVirtualFolder ) {
    if ( collection.resource() == QLatin1String( "akonadi_nepomuktag_resource" ) || collection.resource() == QLatin1String( "akonadi_search_resource" ) )
      return false;
  }
  if ( d->hideSpecificFolder ) {
    QSharedPointer<FolderCollection> col = FolderCollection::forCollection( collection );
    if ( col && col->hideInSelectionDialog() )
      return false;
  }

  return Akonadi::EntityRightsFilterModel::acceptRow( sourceRow, sourceParent );
}


#include "readablecollectionproxymodel.moc"

