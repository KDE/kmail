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

#include <akonadi_next/krecursivefilterproxymodel.h>

#include <QtGui/QApplication>
#include <QtGui/QPalette>
#include <KIcon>

class ReadableCollectionProxyModel::Private
{
public:
  Private()
    : enableCheck( false ) {
  }
  bool enableCheck;
};

ReadableCollectionProxyModel::ReadableCollectionProxyModel( QObject *parent )
  : Akonadi::EntityRightsFilterModel( parent ),
    d( new Private )
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



#include "readablecollectionproxymodel.moc"

