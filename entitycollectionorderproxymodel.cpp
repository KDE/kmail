/* -*- mode: C++; c-file-style: "gnu" -*-
  This file is part of KMail, the KDE mail client.
  Copyright (c) 2010 Montel Laurent <montel@kde.org>

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

#include "entitycollectionorderproxymodel.h"
#include "kmkernel.h"
#include <akonadi/collection.h>
#include <akonadi/entitytreemodel.h>
#include <kdebug.h>

class EntityCollectionOrderProxyModel::EntityCollectionOrderProxyModelPrivate
{
public:
  EntityCollectionOrderProxyModelPrivate()
    : manualSortingActive( false )
  {
  }
  void createOrderSpecialCollection()
  {
    if ( KMKernel::self()->inboxCollectionFolder().id()>0 &&
         KMKernel::self()->outboxCollectionFolder().id()>0&&
         KMKernel::self()->trashCollectionFolder().id()>0&&
         KMKernel::self()->draftsCollectionFolder().id()>0&&
         KMKernel::self()->templatesCollectionFolder().id()>0 &&
         KMKernel::self()->sentCollectionFolder().id()>0)
      {
        orderSpecialCollection<<
          KMKernel::self()->inboxCollectionFolder().id()<<
          KMKernel::self()->outboxCollectionFolder().id()<<
          KMKernel::self()->sentCollectionFolder().id()<<
          KMKernel::self()->trashCollectionFolder().id()<<
          KMKernel::self()->draftsCollectionFolder().id()<<
          KMKernel::self()->templatesCollectionFolder().id();
      }
  }
  bool manualSortingActive;
  QList<qlonglong> orderSpecialCollection;
};



EntityCollectionOrderProxyModel::EntityCollectionOrderProxyModel( QObject *parent )
  : EntityOrderProxyModel( parent ), d( new EntityCollectionOrderProxyModelPrivate() )
{
  setDynamicSortFilter(true);
}

EntityCollectionOrderProxyModel::~EntityCollectionOrderProxyModel()
{
  if ( d->manualSortingActive )
    saveOrder();
  delete d;
}


bool EntityCollectionOrderProxyModel::lessThan( const QModelIndex&left, const QModelIndex & right ) const
{
  if ( !d->manualSortingActive ) {
    if ( d->orderSpecialCollection.isEmpty() ) {
      d->createOrderSpecialCollection();
    }

    if ( !d->orderSpecialCollection.isEmpty() ) {
      Akonadi::Collection::Id leftData = left.data( Akonadi::EntityTreeModel::CollectionIdRole ).toLongLong();
      Akonadi::Collection::Id rightData = right.data( Akonadi::EntityTreeModel::CollectionIdRole ).toLongLong();

      const int leftPos = d->orderSpecialCollection.indexOf( leftData );
      const int rightPos = d->orderSpecialCollection.indexOf( rightData );
      if ( leftPos < 0 && rightPos < 0 )
        return QSortFilterProxyModel::lessThan( left, right );
      else if ( leftPos >= 0 && rightPos < 0)
        return true;
      else if ( leftPos >= 0 && rightPos >=0 )
        return ( leftPos < rightPos );
      else if ( leftPos < 0 && rightPos >= 0 )
        return false;
    }
    return QSortFilterProxyModel::lessThan( left, right );
  }
  return EntityOrderProxyModel::lessThan( left, right );
}

void EntityCollectionOrderProxyModel::setManualSortingActive( bool active )
{
  d->manualSortingActive = active;
  if ( !active ) {
    clearTreeOrder();
  } else {
    invalidate();
  }
}

bool EntityCollectionOrderProxyModel::isManualSortingActive() const
{
  return d->manualSortingActive;
}

#include "entitycollectionorderproxymodel.moc"
