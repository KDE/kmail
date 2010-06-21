/*
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


#include "collectionpane.h"
#include "kmkernel.h"
#include "foldercollection.h"
#include <kpimidentities/identitymanager.h>
#include <kpimidentities/identity.h>
#include <akonadi/kmime/messagefolderattribute.h>

CollectionPane::CollectionPane( QAbstractItemModel *model, QItemSelectionModel *selectionModel, QWidget *parent )
  :MessageList::Pane( model, selectionModel, parent )
{
}

CollectionPane::~CollectionPane()
{
}

MessageList::StorageModel *CollectionPane::createStorageModel( QAbstractItemModel *model, QItemSelectionModel *selectionModel, QObject *parent )
{
  return new CollectionStorageModel( model, selectionModel, parent );
}

CollectionStorageModel::CollectionStorageModel( QAbstractItemModel *model, QItemSelectionModel *selectionModel, QObject *parent )
  : MessageList::StorageModel( model, selectionModel, parent )
{
}

CollectionStorageModel::~CollectionStorageModel()
{
}

bool CollectionStorageModel::isOutBoundFolder( const Akonadi::Collection& c ) const
{
  if ( c.hasAttribute<Akonadi::MessageFolderAttribute>()
       && c.attribute<Akonadi::MessageFolderAttribute>()->isOutboundFolder() ) {
    return true;
  }
  QSharedPointer<FolderCollection> fd = FolderCollection::forCollection( c );
  if ( fd ) {
    // default setting
    const KPIMIdentities::Identity & identity =
      kmkernel->identityManager()->identityForUoidOrDefault( fd->identity() );

    if ( kmkernel->isSystemFolderCollection(c) &&
         !kmkernel->isImapFolder( c ) ) {
      // local system folders
      if ( c == kmkernel->inboxCollectionFolder() ||
           c == kmkernel->trashCollectionFolder() )
        return false;
      if ( c == kmkernel->outboxCollectionFolder() ||
           c == kmkernel->sentCollectionFolder() ||
           c == kmkernel->templatesCollectionFolder() ||
           c == kmkernel->draftsCollectionFolder() )
        return true;
    } else if ( identity.drafts() == fd->idString() ||
                identity.templates() == fd->idString() ||
                identity.fcc() == fd->idString() )
      // drafts, templates or sent of the identity
      return true;
    else
      return false;
  }

  return false;
}


