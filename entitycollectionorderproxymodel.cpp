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
#include "mailcommon.h"

#include <akonadi/collection.h>
#include <akonadi/entitytreemodel.h>
#include <akonadi/kmime/specialmailcollections.h>
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
    if ( mailCommon->inboxCollectionFolder().id()>0 &&
         mailCommon->outboxCollectionFolder().id()>0&&
         mailCommon->trashCollectionFolder().id()>0&&
         mailCommon->draftsCollectionFolder().id()>0&&
         mailCommon->templatesCollectionFolder().id()>0 &&
         mailCommon->sentCollectionFolder().id()>0)
      {
        orderSpecialCollection<<
          mailCommon->inboxCollectionFolder().id()<<
          mailCommon->outboxCollectionFolder().id()<<
          mailCommon->sentCollectionFolder().id()<<
          mailCommon->trashCollectionFolder().id()<<
          mailCommon->draftsCollectionFolder().id()<<
          mailCommon->templatesCollectionFolder().id();
      }
  }
  bool manualSortingActive;
  QList<qlonglong> orderSpecialCollection;
  MailCommon* mailCommon;
};



EntityCollectionOrderProxyModel::EntityCollectionOrderProxyModel( MailCommon* mailCommon, QObject* parent )
  : EntityOrderProxyModel( parent ), d( new EntityCollectionOrderProxyModelPrivate() )
{
  d->mailCommon = mailCommon;
  setDynamicSortFilter(true);
  connect( Akonadi::SpecialMailCollections::self(), SIGNAL( defaultCollectionsChanged() ),
           this, SLOT( slotDefaultCollectionsChanged () ) );

}

EntityCollectionOrderProxyModel::~EntityCollectionOrderProxyModel()
{
  if ( d->manualSortingActive )
    saveOrder();
  delete d;
}


void EntityCollectionOrderProxyModel::slotDefaultCollectionsChanged()
{
  if ( !d->manualSortingActive )
    invalidate();
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
