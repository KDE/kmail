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

class EntityCollectionOrderProxyModel::EntityCollectionOrderProxyModelPrivate
{
public:
  EntityCollectionOrderProxyModelPrivate()
    : manualSortingActive( false )
  {
  }
  bool manualSortingActive;
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
  return EntityOrderProxyModel::lessThan( left, right );
}

void EntityCollectionOrderProxyModel::setManualSortingActive( bool active )
{
  d->manualSortingActive = active;
  if ( !active ) {
    clearTreeOrder();
  }
}

bool EntityCollectionOrderProxyModel::isManualSortingActive() const
{
  return d->manualSortingActive;
}

#include "entitycollectionorderproxymodel.moc"
