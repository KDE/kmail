/*
    Copyright (c) 2009 Laurent Montel <montel@kde.org>


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
#include <akonadi/collection.h>
#include <akonadi/entitytreemodel.h>
#include <kdebug.h>
//TODO needs to export to public class
#include <akonadi/private/collectionutils_p.h>

#include <QtGui/QApplication>
#include <QtGui/QPalette>
#include <KIcon>

class ReadableCollectionProxyModel::Private
{
public:
  Private( ReadableCollectionProxyModel *parent )
    : necessaryRight( Akonadi::Collection::AllRights ),
      mParent( parent ),
      enableCheck( false )
    {
    }
  Akonadi::Collection::Rights necessaryRight;
  ReadableCollectionProxyModel *mParent;
  bool enableCheck;
};

ReadableCollectionProxyModel::ReadableCollectionProxyModel( QObject *parent )
  : QSortFilterProxyModel( parent ),
    d( new Private( this ) )
{
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
    Akonadi::Collection collection = sourceModel()->data( sourceIndex, Akonadi::EntityTreeModel::CollectionRole ).value<Akonadi::Collection>();

    if ( Akonadi::CollectionUtils::isVirtual( collection ) ) {
      return Qt::NoItemFlags;
    }
    if ( !( collection.rights() & d->necessaryRight )) {
      return Qt::NoItemFlags;
    }
  }

  return QSortFilterProxyModel::flags( index );
}

void ReadableCollectionProxyModel::setEnabledCheck( bool enable )
{
  d->enableCheck = enable;
}

bool ReadableCollectionProxyModel::isEnabledCheck() const
{
  return d->enableCheck;
}

void ReadableCollectionProxyModel::setNecessaryRight( Akonadi::Collection::Rights right )
{
  d->necessaryRight = right;
}

Akonadi::Collection::Rights ReadableCollectionProxyModel::necessaryRight() const
{
  return d->necessaryRight;
}

bool ReadableCollectionProxyModel::dropMimeData( const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent )
{
  Q_ASSERT(sourceModel());
  const QModelIndex sourceParent = mapToSource(parent);
  return sourceModel()->dropMimeData(data, action, row, column, sourceParent);
}

QMimeData* ReadableCollectionProxyModel::mimeData( const QModelIndexList & indexes ) const
{
  Q_ASSERT(sourceModel());
  QModelIndexList sourceIndexes;
  foreach(const QModelIndex& index, indexes)
    sourceIndexes << mapToSource(index);
  return sourceModel()->mimeData(sourceIndexes);
}

QStringList ReadableCollectionProxyModel::mimeTypes() const
{
  Q_ASSERT(sourceModel());
  return sourceModel()->mimeTypes();
}

#include "readablecollectionproxymodel.moc"

